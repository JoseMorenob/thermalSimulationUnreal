// Copyright Epic Games, Inc. All Rights Reserved.

// Estas pruebas se compilan solo en una compilacion de desarrollo del editor.
// Verifican los cinco ensayos de validación descritos en la memoria del TFM.
#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "AssetCompilingManager.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/StaticMesh.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "FileHelpers.h"
#include "IRCoreBridge.h"
#include "IRSceneEnvironmentActor.h"
#include "IRThermalDemoObjectActor.h"
#include "IRThermalSurfaceComponent.h"
#include "Materials/MaterialInterface.h"
#include "Math/RotationMatrix.h"
#include "RadianceCaptureActor.h"
#include "RenderingThread.h"
#include "ShaderCompiler.h"
#include "UObject/UnrealType.h"
namespace
{
    // Absorbe la cuantizacion RGBA16F, pero detecta errores de gamma,
    // tonemapping o una formula radiometrica incorrecta.
    constexpr float tolerancia_radiometrica = 0.01f;
    // Escena mínima compartida por las pruebas: una cámara IR y un cubo térmico.
    struct FEscenaDePrueba
    {
        UWorld* mundo = nullptr;
        ARadianceCaptureActor* capturador = nullptr;
        AIRThermalDemoObjectActor* cubo = nullptr;
        UIRThermalSurfaceComponent* superficie = nullptr;
        USceneCaptureComponent2D* camara = nullptr;
        USceneCaptureComponent2D* camara_auxiliar = nullptr;
        UMaterialInterface* material_original = nullptr;

        // Carga el mapa guardado: una camara IR y un unico cubo termico.
        bool Cargar(FAutomationTestBase& prueba)
        {
            mundo = UEditorLoadingAndSavingUtils::LoadMap(TEXT("/Game/Maps/IRBufferValidation"));
            if (!mundo)
            {
                prueba.AddError(TEXT("Falta el mapa /Game/Maps/IRBufferValidation."));
                return false;
            }

            for (TActorIterator<ARadianceCaptureActor> it(mundo); it; ++it)
            {
                capturador = *it;
            }

            int32 numero_de_cubos = 0;
            for (TActorIterator<AIRThermalDemoObjectActor> it(mundo); it; ++it)
            {
                cubo = *it;
                ++numero_de_cubos;
            }

            if (!capturador || !cubo || !cubo->Mesh || !cubo->ThermalSurface || numero_de_cubos != 1)
            {
                prueba.AddError(TEXT("El mapa debe contener exactamente una camara IR y un cubo termico."));
                return false;
            }

            superficie = cubo->ThermalSurface;
            TArray<USceneCaptureComponent2D*> componentes;
            capturador->GetComponents(componentes);
            for (USceneCaptureComponent2D* candidato : componentes)
            {
                if (candidato->GetName() == TEXT("RadianceCapture"))
                {
                    camara = candidato;
                }
                else if (candidato->GetName() == TEXT("AuxiliaryCapture"))
                {
                    camara_auxiliar = candidato;
                }
            }
            if (!camara || !camara_auxiliar)
            {
                prueba.AddError(TEXT("No se encontraron los componentes RadianceCapture y AuxiliaryCapture."));
                return false;
            }

			// Caso de cuerpo negro: n=1 y k=0 producen F=0, por lo que la
			// emisividad direccional es uno en toda la imagen.
            superficie->SetDebugMaterial(LoadObject<UMaterialInterface>(nullptr,
                TEXT("/IRSimPlugin/Materials/M_ThermalSurface.M_ThermalSurface")));
            superficie->SetComplexRefractiveIndex(1.0f, 0.0f);
            superficie->SetAtmosphericExtinctionCoefficient(0.0f);
			capturador->RefreshCapturePipeline();
            material_original = cubo->Mesh->GetMaterial(0);

            prueba.TestNotNull(TEXT("El cubo tiene material termico"), material_original);
            prueba.TestTrue(TEXT("La captura fisica no usa postproceso ni autoexposicion"),
                !camara->ShowFlags.PostProcessing && !camara->ShowFlags.EyeAdaptation);
            return !prueba.HasAnyErrors();
        }

        // La primera captura calienta shaders/PSO. La segunda es la medida.
        void PrepararYCapturar()
        {
            FAssetCompilingManager::Get().FinishAllCompilation();
            if (GShaderCompilingManager)
            {
                GShaderCompilingManager->FinishAllCompilation();
            }
            mundo->UpdateWorldComponents(true, false);
            mundo->SendAllEndOfFrameUpdates();
            FlushRenderingCommands();

            capturador->CaptureRadianceFrame();
            FlushRenderingCommands();

            FAssetCompilingManager::Get().FinishAllCompilation();
            if (GShaderCompilingManager)
            {
                GShaderCompilingManager->FinishAllCompilation();
            }
            capturador->CaptureRadianceFrame();
            FlushRenderingCommands();
        }

        void ComprobarMaterialRestaurado(FAutomationTestBase& prueba) const
        {
            prueba.TestTrue(TEXT("El material termico se restaura despues de los pases auxiliares"),
                cubo->Mesh->GetMaterial(0) == material_original);
        }
    };

    // Lee los nueve pixeles centrales. Todos deben corresponder a la cara del
    // cubo. asi no se aprueba una prueba solo porque algun pixel de la imagen da bien.
    float LeerRegionCentral(FAutomationTestBase& prueba, UTextureRenderTarget2D* target,
        float esperado, float tolerancia, const TCHAR* etiqueta, int32 canal = 0)
    {
        if (!target)
        {
            prueba.AddError(FString::Printf(TEXT("Falta el target de %s."), etiqueta));
            return NAN;
        }

        FReadSurfaceDataFlags opciones(RCM_MinMax);
        opciones.SetLinearToGamma(false); // Nunca convertir la medida a color visible.
        TArray<FLinearColor> pixeles;
        FTextureRenderTargetResource* recurso = target->GameThread_GetRenderTargetResource();
        if (!recurso || !recurso->ReadLinearColorPixels(pixeles, opciones))
        {
            prueba.AddError(FString::Printf(TEXT("No se pudo leer %s."), etiqueta));
            return NAN;
        }

        const int32 x = target->SizeX / 2;
        const int32 y = target->SizeY / 2;
        bool correcto = true;
        for (int32 dy = -1; dy <= 1; ++dy)
        {
            for (int32 dx = -1; dx <= 1; ++dx)
            {
                const float valor = pixeles[(y + dy) * target->SizeX + x + dx].Component(canal);
                correcto &= FMath::IsFinite(valor) && FMath::Abs(valor - esperado) <= tolerancia;
            }
        }

        prueba.TestFalse(TEXT("El target fisico no usa sRGB"), target->SRGB);
        prueba.TestEqual(TEXT("El target fisico usa gamma lineal"), target->TargetGamma, 1.0f);
        prueba.TestTrue(etiqueta, correcto);
        return pixeles[y * target->SizeX + x].Component(canal);
    }

    // Lee un único píxel lineal. El ensayo angular lo usa porque cada píxel ve
    // un punto ligeramente distinto de la superficie y, por tanto, un theta
    // distinto. La comparación física se hace contra ese píxel concreto.
    float LeerPixel(FAutomationTestBase& prueba, UTextureRenderTarget2D* target,
        const FIntPoint& pixel, const TCHAR* etiqueta, int32 canal = 0)
    {
        if (!target || pixel.X < 0 || pixel.X >= target->SizeX || pixel.Y < 0 || pixel.Y >= target->SizeY)
        {
            prueba.AddError(FString::Printf(TEXT("Píxel o target inválido para %s."), etiqueta));
            return NAN;
        }

        FReadSurfaceDataFlags opciones(RCM_MinMax);
        opciones.SetLinearToGamma(false);
        TArray<FLinearColor> pixeles;
        FTextureRenderTargetResource* recurso = target->GameThread_GetRenderTargetResource();
        if (!recurso || !recurso->ReadLinearColorPixels(pixeles, opciones))
        {
            prueba.AddError(FString::Printf(TEXT("No se pudo leer %s."), etiqueta));
            return NAN;
        }

        prueba.TestFalse(TEXT("El target de Fresnel no usa sRGB"), target->SRGB);
        prueba.TestEqual(TEXT("El target de Fresnel usa gamma lineal"), target->TargetGamma, 1.0f);
        return pixeles[pixel.Y * target->SizeX + pixel.X].Component(canal);
    }

    struct FGeometriaDelPixel
    {
        FVector punto;
        FVector normal;
        FVector hacia_camara;
        float coseno_incidencia = 0.0f;
    };

    // Reconstruye la referencia geométrica con un raycast, no con la posición
    // nominal de la cámara. Así se usa el punto que el píxel de AuxiliaryCapture
    // ve realmente, sin modificar la ruta de renderizado ni los materiales.
    bool CalcularGeometriaDelPixel(
        FAutomationTestBase& prueba,
        UWorld* mundo,
        USceneCaptureComponent2D* camara_auxiliar,
        UTextureRenderTarget2D* target,
        const AIRThermalDemoObjectActor* cubo,
        const FIntPoint& pixel,
        FGeometriaDelPixel& geometria_salida)
    {
        if (!mundo || !camara_auxiliar || !target || !cubo || target->SizeX <= 0 || target->SizeY <= 0)
        {
            prueba.AddError(TEXT("No se pudo construir la referencia geométrica del ensayo Fresnel."));
            return false;
        }

        // Unreal usa X como eje de visión de SceneCapture. Se forma el rayo que
        // pasa por el centro del píxel a partir de la misma FOV de la captura.
        const float ancho = static_cast<float>(target->SizeX);
        const float alto = static_cast<float>(target->SizeY);
        const float aspecto = ancho / alto;
        const float tangente_media_fov = FMath::Tan(FMath::DegreesToRadians(camara_auxiliar->FOVAngle * 0.5f));
        const float ndc_x = (2.0f * (static_cast<float>(pixel.X) + 0.5f) / ancho) - 1.0f;
        const float ndc_y = 1.0f - (2.0f * (static_cast<float>(pixel.Y) + 0.5f) / alto);
        const FVector rayo_local(1.0f, ndc_x * tangente_media_fov,
            ndc_y * tangente_media_fov / aspecto);
        const FVector origen = camara_auxiliar->GetComponentLocation();
        const FVector direccion = camara_auxiliar->GetComponentTransform()
            .TransformVectorNoScale(rayo_local).GetSafeNormal();

        FHitResult impacto;
        FCollisionQueryParams parametros(SCENE_QUERY_STAT(IRFresnelAngular), false);
        parametros.AddIgnoredActor(camara_auxiliar->GetOwner());
        const bool impacto_encontrado = mundo->LineTraceSingleByChannel(
            impacto, origen, origen + direccion * 100000.0f, ECC_Visibility, parametros);
        if (!impacto_encontrado || impacto.GetActor() != cubo)
        {
            prueba.AddError(TEXT("El rayo del píxel de prueba no impactó en el cubo térmico."));
            return false;
        }

        geometria_salida.punto = impacto.ImpactPoint;
        geometria_salida.normal = impacto.ImpactNormal.GetSafeNormal();
        geometria_salida.hacia_camara = (origen - impacto.ImpactPoint).GetSafeNormal();
        geometria_salida.coseno_incidencia = FMath::Clamp(FVector::DotProduct(
            geometria_salida.normal, geometria_salida.hacia_camara), 0.0f, 1.0f);
        return true;
    }

    // Los parámetros de AIRSceneEnvironmentActor se mantienen protegidos para
    // su edición desde Unreal. Las pruebas los fijan por reflexión para crear
    // escenarios reproducibles sin alterar los valores de la escena de demo.
    bool AsignarFloatDeEntorno(FAutomationTestBase& prueba, AIRSceneEnvironmentActor* entorno,
        const TCHAR* nombre_propiedad, float valor)
    {
        FFloatProperty* propiedad = entorno
            ? FindFProperty<FFloatProperty>(entorno->GetClass(), nombre_propiedad) : nullptr;
        if (!propiedad)
        {
            prueba.AddError(FString::Printf(TEXT("No se encontró el parámetro ambiental %s."), nombre_propiedad));
            return false;
        }

        propiedad->SetPropertyValue_InContainer(entorno, valor);
        return true;
    }

    // Comprueba que la camara no transforma una magnitud fisica en una imagen
    // para pantalla antes de escribirla en el Render Target.
    void ComprobarContratoDeRadiancia(FAutomationTestBase& prueba, const FEscenaDePrueba& escena)
    {
        UTextureRenderTarget2D* target = escena.capturador->GetRadianceRenderTarget();
        prueba.TestTrue(TEXT("La camara escribe en el target de radiancia"), escena.camara->TextureTarget == target);
        prueba.TestTrue(TEXT("La radiancia se guarda como RGBA16F"), target && target->GetFormat() == PF_FloatRGBA);
        prueba.TestEqual(TEXT("La captura usa escena HDR lineal"), escena.camara->CaptureSource, SCS_SceneColorHDRNoAlpha);
        prueba.TestFalse(TEXT("El tonemapper esta desactivado"), escena.camara->ShowFlags.Tonemapper);
    }
}

// TEST 1. La radiancia GPU debe coincidir con la referencia CPU.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIRRadianciaCpuGpuTest, "IRSimClean.Radiance.CpuGpu",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIRRadianciaCpuGpuTest::RunTest(const FString&)
{
    FEscenaDePrueba escena;
    if (!escena.Cargar(*this)) return false;

    for (const float temperatura : {280.0f, 300.0f, 340.0f})
    {
        // Banda LWIR 8--12 um, con los mismos 40 pasos que emplea el material.
        const float referencia_cpu = irsim::core::ComputeBandRadiance(temperatura, 8.0f, 12.0f, 40);
        escena.superficie->SetTemperatureKelvin(temperatura);
        escena.PrepararYCapturar();
        ComprobarContratoDeRadiancia(*this, escena);

        const float medida_gpu = LeerRegionCentral(*this, escena.capturador->GetRadianceRenderTarget(),
            referencia_cpu, FMath::Abs(referencia_cpu) * tolerancia_radiometrica,
            TEXT("Los 9 pixeles de radiancia coinciden con CPU"));
        const float error_relativo = FMath::Abs(medida_gpu - referencia_cpu) /
            FMath::Max(FMath::Abs(referencia_cpu), 1.0e-8f);
        TestTrue(TEXT("Error relativo CPU--GPU menor o igual al 1 %"), error_relativo <= tolerancia_radiometrica);
        AddInfo(FString::Printf(
            TEXT("CPU--GPU: T=%.0f K, CPU=%.6f, GPU=%.6f, error relativo=%.4f %%."),
            temperatura, referencia_cpu, medida_gpu, error_relativo * 100.0f));
    }
    return !HasAnyErrors();
}

// TEST 2. Beer--Lambert en GPU: tau = exp(-k*d), con d=50 m.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIRAtmosferaCpuGpuTest, "IRSimClean.Radiance.Atmosphere",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIRAtmosferaCpuGpuTest::RunTest(const FString&)
{
    FEscenaDePrueba escena;
    if (!escena.Cargar(*this)) return false;

    // Centro a 5250 cm y semiextension 250 cm: la cara esta a 5000 cm = 50 m.
    escena.cubo->SetActorLocation(FVector(5250.0f, 0.0f, 0.0f));
    AIRSceneEnvironmentActor* entorno = escena.mundo->SpawnActor<AIRSceneEnvironmentActor>();
    FFloatProperty* temperatura_aire = entorno
        ? FindFProperty<FFloatProperty>(entorno->GetClass(), TEXT("AirTemperatureK")) : nullptr;
    if (!temperatura_aire)
    {
        AddError(TEXT("No se pudo crear el entorno atmosferico de prueba."));
        return false;
    }

    // Aire a 0 K: no emite radiancia; solo se mide la atenuacion.
    temperatura_aire->SetPropertyValue_InContainer(entorno, 0.0f);
    escena.superficie->ApplySceneEnvironment(entorno);
    escena.superficie->SetTemperatureKelvin(340.0f);
    const float radiancia_origen = irsim::core::ComputeBandRadiance(340.0f, 8.0f, 12.0f, 40);

    for (const float k : {0.0f, 0.001f, 0.015f})
    {
        const float tau = FMath::Exp(-k * 50.0f);
        escena.superficie->SetAtmosphericExtinctionCoefficient(k);
        escena.PrepararYCapturar();
        LeerRegionCentral(*this, escena.capturador->GetDepthRenderTarget(), 5000.0f, 1.0f,
            TEXT("La cara del cubo esta a 50 metros"));
        ComprobarContratoDeRadiancia(*this, escena);
        const float medida_gpu = LeerRegionCentral(*this, escena.capturador->GetRadianceRenderTarget(),
            radiancia_origen * tau, radiancia_origen * tau * tolerancia_radiometrica,
            TEXT("La radiancia atenuada coincide con Beer--Lambert"));
        const float referencia = radiancia_origen * tau;
        const float error_relativo = FMath::Abs(medida_gpu - referencia) /
            FMath::Max(FMath::Abs(referencia), 1.0e-8f);
        AddInfo(FString::Printf(
            TEXT("Beer--Lambert: k=%.3f m^-1, tau=%.6f, referencia=%.6f, GPU=%.6f, error relativo=%.4f %%."),
            k, tau, referencia, medida_gpu, error_relativo * 100.0f));
    }
    return !HasAnyErrors();
}

// TEST 3. Los buffers auxiliares se actualizan entre capturas. El material
// M_IR_Output_Emissivity debe escribir epsilon_dir = 1 - Fresnel(n, k, theta).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIRBuffersAuxiliaresTest, "IRSimClean.Radiance.AuxiliaryBuffers",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIRBuffersAuxiliaresTest::RunTest(const FString&)
{
    FEscenaDePrueba escena;
    if (!escena.Cargar(*this)) return false;

    const float temperaturas[] = {340.0f, 300.0f};
    const float indices_reales[] = {1.0f, 1.5f};
    const float indices_imaginarios[] = {0.0f, 0.0f};
    for (int32 fotograma = 0; fotograma < 2; ++fotograma)
    {
        escena.superficie->SetTemperatureKelvin(temperaturas[fotograma]);
        escena.superficie->SetComplexRefractiveIndex(
            indices_reales[fotograma], indices_imaginarios[fotograma]);
        escena.PrepararYCapturar();

        const float temperatura_gpu = LeerRegionCentral(*this, escena.capturador->GetTemperatureRenderTarget(),
            temperaturas[fotograma], 0.5f, TEXT("Buffer de temperatura en kelvin"));
        const float reflectividad_esperada = irsim::core::ComputeFresnelConductorReflectance(
            1.0f, indices_reales[fotograma], indices_imaginarios[fotograma]);
        const float emisividad_gpu = LeerRegionCentral(*this, escena.capturador->GetEmissivityRenderTarget(),
            1.0f - reflectividad_esperada, 0.002f,
            TEXT("Buffer de emisividad direccional en incidencia normal"));
        const float material_gpu = LeerRegionCentral(*this, escena.capturador->GetMaterialIdRenderTarget(),
            7.0f, 0.001f, TEXT("Buffer de identificador de material"));
        const float profundidad_gpu = LeerRegionCentral(*this, escena.capturador->GetDepthRenderTarget(),
            750.0f, 1.0f, TEXT("Buffer de profundidad en centimetros"));
        LeerRegionCentral(*this, escena.capturador->GetNormalRenderTarget(),
            -1.0f, 0.01f, TEXT("Componente X de la normal mundo"), 0);
        LeerRegionCentral(*this, escena.capturador->GetNormalRenderTarget(),
            0.0f, 0.01f, TEXT("Componente Y de la normal mundo"), 1);
        LeerRegionCentral(*this, escena.capturador->GetNormalRenderTarget(),
            0.0f, 0.01f, TEXT("Componente Z de la normal mundo"), 2);
        AddInfo(FString::Printf(
            TEXT("Buffers auxiliares: T esperada/GPU=%.0f/%.3f K, epsilon esperada/GPU=%.6f/%.6f, ID GPU=%.3f, profundidad GPU=%.3f cm."),
            temperaturas[fotograma], temperatura_gpu, 1.0f - reflectividad_esperada,
            emisividad_gpu, material_gpu, profundidad_gpu));
        escena.ComprobarMaterialRestaurado(*this);
    }
    return !HasAnyErrors();
}

// TEST 5. La radiancia de camino se evalúa con aire emisor, no con el caso
// límite de aire a 0 K empleado para aislar Beer--Lambert en el TEST 2.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIRAtmosferaEmisoraTest, "IRSimClean.Radiance.AtmosphereEmission",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIRAtmosferaEmisoraTest::RunTest(const FString&)
{
    FEscenaDePrueba escena;
    if (!escena.Cargar(*this)) return false;

    // La cara visible queda a 50 m. n=1 elimina Fresnel y el cielo, de modo
    // que la comparación aísla exactamente tau*L_surface + (1-tau)*L_air.
    escena.cubo->SetActorLocation(FVector(5250.0f, 0.0f, 0.0f));
    AIRSceneEnvironmentActor* entorno = escena.mundo->SpawnActor<AIRSceneEnvironmentActor>();
    if (!AsignarFloatDeEntorno(*this, entorno, TEXT("AirTemperatureK"), 280.0f)
        || !AsignarFloatDeEntorno(*this, entorno, TEXT("AtmosphericExtinctionCoefficient"), 0.015f))
    {
        return false;
    }

    escena.superficie->ApplySceneEnvironment(entorno);
    escena.superficie->SetComplexRefractiveIndex(1.0f, 0.0f);
    escena.superficie->SetTemperatureKelvin(340.0f);
    escena.PrepararYCapturar();

    constexpr float distancia_m = 50.0f;
    const float tau = irsim::core::ComputeAtmosphericTransmittance(0.015f, distancia_m);
    const float radiancia_superficie = irsim::core::ComputeBandRadiance(340.0f, 8.0f, 12.0f, 40);
    const float radiancia_aire = irsim::core::ComputeBandRadiance(280.0f, 8.0f, 12.0f, 40);
    const float referencia = irsim::core::ComputeSensorBandRadiance(
        radiancia_superficie, radiancia_aire, tau);
    const float sin_radiancia_camino = tau * radiancia_superficie;

    const float medida_gpu = LeerRegionCentral(*this, escena.capturador->GetRadianceRenderTarget(),
        referencia, referencia * tolerancia_radiometrica,
        TEXT("La radiancia con aire emisor coincide con la ecuación atmosférica completa"));
    TestTrue(TEXT("La radiancia de camino atmosférica es distinta de cero"),
        FMath::Abs(referencia - sin_radiancia_camino) > 0.01f);
    AddInfo(FString::Printf(
        TEXT("Atmósfera emisora: tau=%.6f, L_superficie=%.6f, L_aire=%.6f, referencia=%.6f, GPU=%.6f."),
        tau, radiancia_superficie, radiancia_aire, referencia, medida_gpu));
    return !HasAnyErrors();
}

// TEST 4. La emisividad almacenada no es un parámetro fijo: debe responder al
// ángulo de observación mediante epsilon_dir = 1 - Fresnel(n, k, theta).
//
// Se observa el centro de la cara -X del cubo desde dos posiciones. La primera
// es perpendicular a la cara y la segunda forma 60 grados con su normal. En
// ambos casos la cámara apunta al mismo punto, por lo que el píxel central mide
// la misma superficie y el único cambio físico intencionado es theta.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIRFresnelAngularTest, "IRSimClean.Radiance.FresnelAngular",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIRFresnelAngularTest::RunTest(const FString&)
{
    FEscenaDePrueba escena;
    if (!escena.Cargar(*this)) return false;

    UStaticMesh* malla_estatica = escena.cubo->Mesh->GetStaticMesh();
    if (!malla_estatica)
    {
        AddError(TEXT("El cubo de prueba no tiene una malla estática."));
        return false;
    }

    // El mapa de validación usa un cubo sin rotar. Calculamos el punto y la
    // normal desde la malla para que la geometría del ensayo quede explícita.
    const FBox limites_locales = malla_estatica->GetBoundingBox();
    const FVector centro_cara_local(
        limites_locales.Min.X,
        (limites_locales.Min.Y + limites_locales.Max.Y) * 0.5f,
        (limites_locales.Min.Z + limites_locales.Max.Z) * 0.5f);
    const FTransform transformacion_malla = escena.cubo->Mesh->GetComponentTransform();
    const FVector punto_superficie = transformacion_malla.TransformPosition(centro_cara_local);
    const FVector normal_superficie = transformacion_malla
        .TransformVectorNoScale(FVector(-1.0f, 0.0f, 0.0f)).GetSafeNormal();
    const FVector direccion_lateral = FVector::CrossProduct(FVector::UpVector, normal_superficie).GetSafeNormal();
    if (normal_superficie.IsNearlyZero() || direccion_lateral.IsNearlyZero())
    {
        AddError(TEXT("No se pudo construir la geometría de la prueba angular de Fresnel."));
        return false;
    }

    // n=1.5, k=0 hace visible la variación de Fresnel entre incidencia normal
    // y oblicua, sin introducir absorción del índice imaginario.
    constexpr float indice_real = 1.5f;
    constexpr float indice_imaginario = 0.0f;
    constexpr float distancia_normal_cm = 1500.0f;
    escena.superficie->SetComplexRefractiveIndex(indice_real, indice_imaginario);
    escena.superficie->SetAtmosphericExtinctionCoefficient(0.0f);

    struct FCasoAngular
    {
        const TCHAR* nombre;
        float desplazamiento_lateral_cm;
        float coseno_nominal;
    };

    // tan(60 grados) = raiz(3). La segunda posición es oblicua respecto de
    // la cara; el coseno exacto se reconstruye después mediante el raycast.
    const FCasoAngular casos[] = {
        {TEXT("incidencia normal"), 0.0f, 1.0f},
        {TEXT("incidencia oblicua de 60 grados"), FMath::Sqrt(3.0f) * distancia_normal_cm, 0.5f}
    };

    float emisividad_normal = NAN;
    float emisividad_oblicua = NAN;
    for (const FCasoAngular& caso : casos)
    {
        const FVector posicion_camara = punto_superficie
            + normal_superficie * distancia_normal_cm
            + direccion_lateral * caso.desplazamiento_lateral_cm;

        // En Unreal el eje X local de SceneCapture es su dirección de visión.
        // Al apuntarlo al punto de la cara, dicho punto cae en el píxel central.
        const FRotator rotacion_camara = FRotationMatrix::MakeFromX(
            punto_superficie - posicion_camara).Rotator();
        escena.capturador->SetActorLocationAndRotation(posicion_camara, rotacion_camara);

        // El buffer se genera con AuxiliaryCapture, no con RadianceCapture.
        // Aunque ambos deberían estar alineados, la referencia debe usar la
        // vista que Unreal emplea realmente al evaluar CameraVectorWS.
        escena.PrepararYCapturar();
        UTextureRenderTarget2D* target_emisividad = escena.capturador->GetEmissivityRenderTarget();
        const FIntPoint pixel_central(
            target_emisividad ? target_emisividad->SizeX / 2 : 0,
            target_emisividad ? target_emisividad->SizeY / 2 : 0);
        FGeometriaDelPixel geometria;
        if (!CalcularGeometriaDelPixel(*this, escena.mundo, escena.camara_auxiliar,
            target_emisividad, escena.cubo, pixel_central, geometria))
        {
            return false;
        }
        const float reflectividad_esperada = irsim::core::ComputeFresnelConductorReflectance(
            geometria.coseno_incidencia, indice_real, indice_imaginario);
        const float emisividad_esperada = 1.0f - reflectividad_esperada;
        const FString etiqueta_buffer = FString::Printf(
            TEXT("El buffer coincide con Fresnel en %s"), caso.nombre);
        const float emisividad_gpu = LeerPixel(*this, target_emisividad, pixel_central, *etiqueta_buffer);
        TestTrue(*etiqueta_buffer,
            FMath::IsFinite(emisividad_gpu) && FMath::Abs(emisividad_gpu - emisividad_esperada) <= 0.002f);
        // La lectura de normal confirma que el píxel central sigue en la misma
        // cara plana; evita atribuir a Fresnel un cambio de cara del cubo.
        const FVector normal_buffer(
            LeerRegionCentral(*this, escena.capturador->GetNormalRenderTarget(),
                normal_superficie.X, 0.01f, TEXT("Normal X de la cara medida"), 0),
            LeerRegionCentral(*this, escena.capturador->GetNormalRenderTarget(),
                normal_superficie.Y, 0.01f, TEXT("Normal Y de la cara medida"), 1),
            LeerRegionCentral(*this, escena.capturador->GetNormalRenderTarget(),
                normal_superficie.Z, 0.01f, TEXT("Normal Z de la cara medida"), 2));
        AddInfo(FString::Printf(
            TEXT("Fresnel angular (%s): cos(theta) nominal=%.3f, cos(theta) por raycast=%.3f, normal GPU=(%.3f, %.3f, %.3f), epsilon esperada=%.6f, epsilon GPU=%.6f."),
            caso.nombre, caso.coseno_nominal, geometria.coseno_incidencia, normal_buffer.X, normal_buffer.Y, normal_buffer.Z,
            emisividad_esperada, emisividad_gpu));

        if (caso.desplazamiento_lateral_cm == 0.0f)
        {
            emisividad_normal = emisividad_gpu;
        }
        else
        {
            emisividad_oblicua = emisividad_gpu;
        }
    }

    // Para este material, Fresnel aumenta al pasar a visión rasante; por tanto,
    // la emisividad direccional debe reducirse. Esta comparación detecta que el
    // material ignorase CameraPosition o usase un valor escalar fijo.
    TestTrue(TEXT("La emisividad direccional disminuye al aumentar el ángulo"),
        FMath::IsFinite(emisividad_normal) && FMath::IsFinite(emisividad_oblicua)
        && emisividad_oblicua < emisividad_normal - 0.01f);

    // La memoria incluye una captura adicional a incidencia normal con
    // kappa=2 para verificar que el material no ignora la parte imaginaria
    // del índice de refracción. Pertenece al mismo ensayo de Fresnel.
    constexpr float indice_imaginario_conductor = 2.0f;
    const FVector posicion_normal = punto_superficie + normal_superficie * distancia_normal_cm;
    escena.capturador->SetActorLocationAndRotation(posicion_normal,
        FRotationMatrix::MakeFromX(punto_superficie - posicion_normal).Rotator());
    escena.superficie->SetComplexRefractiveIndex(indice_real, indice_imaginario_conductor);
    escena.PrepararYCapturar();
    const float reflectividad_conductor = irsim::core::ComputeFresnelConductorReflectance(
        1.0f, indice_real, indice_imaginario_conductor);
    const float emisividad_conductor_esperada = 1.0f - reflectividad_conductor;
    const float emisividad_conductor_gpu = LeerRegionCentral(*this,
        escena.capturador->GetEmissivityRenderTarget(), emisividad_conductor_esperada,
        0.002f, TEXT("Fresnel conductor con kappa positivo"));
    AddInfo(FString::Printf(
        TEXT("Fresnel conductor: n=%.1f, kappa=%.1f, epsilon esperada=%.6f, epsilon GPU=%.6f."),
        indice_real, indice_imaginario_conductor, emisividad_conductor_esperada,
        emisividad_conductor_gpu));

    escena.ComprobarMaterialRestaurado(*this);
    return !HasAnyErrors();
}

#endif

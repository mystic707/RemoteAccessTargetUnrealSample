// Copyright Epic Games, Inc. All Rights Reserved.

#include "RemoteAccessTest003Character.h"
#include "RemoteAccessTest003Projectile.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputSubsystemInterface.h"
#include "InputActionValue.h"
#include "Engine/LocalPlayer.h"
#include "SocketSubsystem.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include <string>
#include "GameFramework/PlayerController.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// ARemoteAccessTest003Character

ARemoteAccessTest003Character::ARemoteAccessTest003Character()
{
	PrimaryActorTick.bCanEverTick = true;		// TODO : 
	Socket = nullptr;							// TODO : 

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);
		
	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-10.f, 0.f, 60.f)); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	//Mesh1P->SetRelativeRotation(FRotator(0.9f, -19.19f, 5.2f));
	Mesh1P->SetRelativeLocation(FVector(-30.f, 0.f, -150.f));

}

void ARemoteAccessTest003Character::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	// TODO : 
	StartTCPReceiver();

	// TODO : 
	// Get the Player Controller
    APlayerController* PlayerController = Cast<APlayerController>(GetController());
    if (PlayerController && InputMappingContext)
    {
		UE_LOG(LogTemp, Log, TEXT("BeginPlay AddMappingContext 001"));
        // Add the Input Mapping Context
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
        {
            Subsystem->AddMappingContext(InputMappingContext, 0);
			UE_LOG(LogTemp, Log, TEXT("BeginPlay AddMappingContext 002"));
        }
    }
}

void ARemoteAccessTest003Character::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);

    APlayerController* PlayerController = Cast<APlayerController>(NewController);
    if (PlayerController && InputMappingContext)
    {
        // Add the Input Mapping Context
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
        {
            Subsystem->AddMappingContext(InputMappingContext, 0);
        }
    }
}

void ARemoteAccessTest003Character::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (Socket && Socket->GetConnectionState() == ESocketConnectionState::SCS_Connected)
    {
        ReceiveData();
    }
}

//////////////////////////////////////////////////////////////////////////// Input

void ARemoteAccessTest003Character::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{	
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ARemoteAccessTest003Character::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ARemoteAccessTest003Character::Look);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}


void ARemoteAccessTest003Character::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add movement 
		AddMovementInput(GetActorForwardVector(), MovementVector.Y);
		AddMovementInput(GetActorRightVector(), MovementVector.X);
	}
}

void ARemoteAccessTest003Character::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void ARemoteAccessTest003Character::ReleaseKey()
{
    APlayerController* PlayerController = Cast<APlayerController>(GetController());
    if (PlayerController)
    {
        // Simulate the key release
        PlayerController->InputKey(KeyToRelease, EInputEvent::IE_Released, 1.0f, false);
		UE_LOG(LogTemp, Log, TEXT("ReleaseKey done"));
    }
}

void ARemoteAccessTest003Character::SimulateKeyMappingPress(FKey Key)
{
    APlayerController* PlayerController = Cast<APlayerController>(GetController());
    if (PlayerController)
    {
        // Simulate the param 'Key' press
        PlayerController->InputKey(Key, EInputEvent::IE_Pressed, 1.0f, false);

		// Store the key to be released
		KeyToRelease = Key;

		// 타이머 시간 이후 key release 처리
		GetWorld()->GetTimerManager().SetTimer(
			KeyReleaseTimerHandle,
            this,
            &ARemoteAccessTest003Character::ReleaseKey,
            0.5f,
            false
        );
    }
}

void ARemoteAccessTest003Character::StartTCPReceiver()
{
    Socket = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket(NAME_Stream, TEXT("default"), false);

    FIPv4Address ip;
    FIPv4Address::Parse(TEXT("10.10.113.81"), ip);		// TODO : 서버 ip 정보 확인

    TSharedRef<FInternetAddr> addr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
    addr->SetIp(ip.Value);
    addr->SetPort(12345);			// TODO : 서버 포트 정보 확인

    bool connected = Socket->Connect(*addr);
    if (!connected)
    {
        UE_LOG(LogTemp, Log, TEXT("Failed to connect to server"));
    }
	else
	{
		UE_LOG(LogTemp, Log, TEXT("Success to connect to server"));
	}
}

void ARemoteAccessTest003Character::ReceiveData()
{
    uint32 Size;
    while (Socket->HasPendingData(Size))
    {
        ReceivedData.SetNumUninitialized(FMath::Min(Size, 65507u));

        int32 Read = 0;
        Socket->Recv(ReceivedData.GetData(), ReceivedData.Num(), Read);

        const std::string cstr(reinterpret_cast<const char*>(ReceivedData.GetData()), Read);
        FString ReceivedString = FString(cstr.c_str());

		// 
		if(ReceivedString == TEXT("up")) {
			AddMovementInput(GetActorForwardVector(), 4000.0f);
		}
		else if(ReceivedString == TEXT("down")) {
			AddMovementInput(GetActorForwardVector(), -4000.0f);
		}
		else if(ReceivedString == TEXT("left")) {
			AddMovementInput(GetActorRightVector(), -4000.0f);
		}
		else if(ReceivedString == TEXT("right")) {
			AddMovementInput(GetActorRightVector(), 4000.0f);
		}
		else if(ReceivedString == TEXT("ActionA")) {
			Jump();
		}
		// Key Mapping Input
		else if(ReceivedString == TEXT("w")) {
			SimulateKeyMappingPress(EKeys::W);
		}
		else if(ReceivedString == TEXT("a")) {
			SimulateKeyMappingPress(EKeys::A);
		}
		else if(ReceivedString == TEXT("d")) {
			SimulateKeyMappingPress(EKeys::D);
		}
		else if(ReceivedString == TEXT("s")) {
			SimulateKeyMappingPress(EKeys::S);
		}

        UE_LOG(LogTemp, Log, TEXT("Received: %s"), *ReceivedString);
    }
}
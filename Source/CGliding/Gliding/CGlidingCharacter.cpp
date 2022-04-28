// Copyright Epic Games, Inc. All Rights Reserved.

#include "CGlidingCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"

//////////////////////////////////////////////////////////////////////////
// ACGlidingCharacter



void ACGlidingCharacter::ToggleGlide()
{
	if(bCanGlide && !bIsGliding)
	{
		StartGlide();
	}
	else
	{
		StopGlide();
	}
}


void ACGlidingCharacter::StartGlide()
{
	GetCharacterMovement()->SetMovementMode(MOVE_Flying);
	bIsGliding = true;
}

void ACGlidingCharacter::StopGlide()
{
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	bIsGliding = false;
	bCanGlide = true;
}

void ACGlidingCharacter::StartJump()
{
	if(GetCharacterMovement()->IsFalling())
	{
		ToggleGlide();
	}
	else
	{
		Jump();
	}
}

void ACGlidingCharacter::StopJump()
{
	// default jumping stop behaviour
	StopJumping();
}

void ACGlidingCharacter::UpdateDistanceToGround()
{
	CalculateDistanceToGround();

	if(DistanceToGround > 20.0f)
	{
		bCanGlide = true;
	}
	else
	{
		StopGlide();
	}
}

void ACGlidingCharacter::UpdateGlidingPosition(float DeltaTime)
{
	const FVector ActorLocation = GetActorLocation();
	const FVector FallLocation = FVector::DownVector * -DownwardForce;
	const FVector TargetLocation = ActorLocation - FallLocation;
	
	const FVector NewLocation = (DeltaTime != 0.0f) ?
		FMath::VInterpTo(ActorLocation, TargetLocation, DeltaTime, 5.0f) : ActorLocation;

	SetActorLocation(NewLocation);
}


void ACGlidingCharacter::CalculateDistanceToGround()
{
	const FVector Position = GroundBoxComponent->GetComponentLocation();
	const FVector EndOffset = Position * FVector::DownVector * 2000.0f;
	FHitResult Result;

	FCollisionQueryParams GroundTraceParams = FCollisionQueryParams(FName(TEXT("GroundTrace")), false, this);
	GetWorld()->LineTraceSingleByChannel(Result, Position, EndOffset, ECC_Camera, GroundTraceParams);

	DistanceToGround = Result.Distance;
}

void ACGlidingCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if(bIsGliding)
	{
		UpdateDistanceToGround();
		UpdateGlidingPosition(DeltaSeconds);
	}
}


ACGlidingCharacter::ACGlidingCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rate for input
	TurnRateGamepad = 50.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	GroundBoxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("GroundBox"));
	GroundBoxComponent->SetupAttachment(RootComponent);
	
	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

//////////////////////////////////////////////////////////////////////////
// Input

void ACGlidingCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACGlidingCharacter::StartJump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACGlidingCharacter::StopJump);

	PlayerInputComponent->BindAxis("Move Forward / Backward", this, &ACGlidingCharacter::MoveForward);
	PlayerInputComponent->BindAxis("Move Right / Left", this, &ACGlidingCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn Right / Left Mouse", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("Turn Right / Left Gamepad", this, &ACGlidingCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("Look Up / Down Mouse", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("Look Up / Down Gamepad", this, &ACGlidingCharacter::LookUpAtRate);

	// handle touch devices
	PlayerInputComponent->BindTouch(IE_Pressed, this, &ACGlidingCharacter::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &ACGlidingCharacter::TouchStopped);
}

void ACGlidingCharacter::BeginPlay()
{
	Super::BeginPlay();
	//
	// GroundBoxComponent->OnComponentBeginOverlap.AddDynamic(this, &ACGlidingCharacter::OnGroundOverlapBegin);
	// GroundBoxComponent->OnComponentEndOverlap.AddDynamic(this, &ACGlidingCharacter::OnGroundOverlapEnd);
}

void ACGlidingCharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
	Jump();
}

void ACGlidingCharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
	StopJumping();
}

void ACGlidingCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * TurnRateGamepad * GetWorld()->GetDeltaSeconds());
}

void ACGlidingCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * TurnRateGamepad * GetWorld()->GetDeltaSeconds());
}

void ACGlidingCharacter::MoveForward(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void ACGlidingCharacter::MoveRight(float Value)
{
	if ( (Controller != nullptr) && (Value != 0.0f) )
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}

// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "CGlidingCharacter.generated.h"

UCLASS(config=Game)
class ACGlidingCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;




public:
	ACGlidingCharacter();

	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Input)
	float TurnRateGamepad;

protected:

	/** Called for forwards/backward input */
	void MoveForward(float Value);

	/** Called for side to side input */
	void MoveRight(float Value);

	/** 
	 * Called via input to turn at a given rate. 
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void TurnAtRate(float Rate);

	/**
	 * Called via input to turn look up/down at a given rate. 
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void LookUpAtRate(float Rate);

	/** Handler for when a touch input begins. */
	void TouchStarted(ETouchIndex::Type FingerIndex, FVector Location);

	/** Handler for when a touch input stops. */
	void TouchStopped(ETouchIndex::Type FingerIndex, FVector Location);

protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	// End of APawn interface

public:

	virtual void BeginPlay() override;
	
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }



	// GLIDING STUFF HERE
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Gliding, meta = (AllowPrivateAccess = "true"))
	class UBoxComponent* GroundBoxComponent; // set to private later

	UPROPERTY()
	float DownwardForce = 50.0f;

	UPROPERTY()
	float DistanceToGround = 0.0f;
	
	UPROPERTY()
	bool bCanGlide = true;

	
	UPROPERTY()
	bool bIsGliding = false;
	
	UFUNCTION()
	void ToggleGlide();

	UFUNCTION()
	void StartGlide();

	UFUNCTION()
	void StopGlide();
	
	UFUNCTION()
	void StartJump();

	UFUNCTION()
	void StopJump();

	UFUNCTION()
	void UpdateDistanceToGround();

	UFUNCTION()
	void UpdateGlidingPosition(float DeltaTime);
	
	UFUNCTION()
	void CalculateDistanceToGround();
	
	virtual void Tick(float DeltaSeconds) override;
};

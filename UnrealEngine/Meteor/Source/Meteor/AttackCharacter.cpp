// Copyright IceRiver. All Rights Reserved.

#include "AttackCharacter.h"

#include "Animation/AnimInstance.h"
#include "Engine.h"
#include "GameFramework/Character.h"
#include "Components/BoxComponent.h"

#include "Meteor.h"
#include "Meteor/Common/MeteorFuncLib.h"
#include "Meteor/Components/InputBufferComponent.h"
#include "Meteor/Components/InputCommandComponent.h"
#include "Meteor/Components/CombatSystemComponent.h"

AAttackCharacter::AAttackCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	MoveFwdSpeedFactor = 1.0f;

	MoveBwdSpeedFactor = 0.3f;

	MoveRightSpeedFactor = 0.3f;

	InputCommandCP = CreateDefaultSubobject<UInputCommandComponent>(TEXT("InputCommand"));
	AddTickPrerequisiteComponent(InputCommandCP);

	CombatSystemCP = CreateDefaultSubobject<UCombatSystemComponent>(TEXT("CombatSystem"));
	AddTickPrerequisiteComponent(CombatSystemCP);
	CombatSystemCP->SetInputCommandComponent(InputCommandCP);

	/*bau_Head_Test = CreateDefaultSubobject<UBoxComponent>(TEXT("bau_Head_Test"));
	bau_Head_Test->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, "bau_Head");
	bau_Head_Test->SetBoxExtent(FVector(12.0f, 10.0f, 10.0f));
	bau_Head_Test->SetRelativeLocation(FVector(6.0f, 0.0f, 0.0f));
	bau_Head_Test->bGenerateOverlapEvents = true;
	bau_Head_Test->SetCollisionObjectType(HitBoxObjectChannel);
	bau_Head_Test->SetCollisionProfileName("HitBoxCheck");*/
}

void AAttackCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void AAttackCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AAttackCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	check(PlayerInputComponent);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AAttackCharacter::OnJump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &AAttackCharacter::StopJump);

	PlayerInputComponent->BindAction("Guard", IE_Pressed, InputCommandCP, &UInputCommandComponent::OnGuard);
	PlayerInputComponent->BindAction("Guard", IE_Released, InputCommandCP, &UInputCommandComponent::StopGuard);

	PlayerInputComponent->BindAxis("MoveForward", this, &AAttackCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AAttackCharacter::MoveRight);

	PlayerInputComponent->BindAxis("Turn", this, &AAttackCharacter::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &AAttackCharacter::LookUp);
}

void AAttackCharacter::MoveForward(float Value)
{
	if (CombatSystemCP->GetMoveType() == MOVE_TYPE::MOVE_IDLE || CombatSystemCP->GetMoveType() == MOVE_TYPE::MOVE_JUMP)
	{
		if (Value > 0.0f)
			Value *= MoveFwdSpeedFactor;
		else
			Value *= MoveBwdSpeedFactor;

		AddMovementInput(GetActorForwardVector(), Value);
	}
}

void AAttackCharacter::MoveRight(float Value)
{
	if (CombatSystemCP->GetMoveType() == MOVE_TYPE::MOVE_IDLE || CombatSystemCP->GetMoveType() == MOVE_TYPE::MOVE_JUMP)
	{
		AddMovementInput(GetActorRightVector(), Value * MoveRightSpeedFactor);
	}
}

void AAttackCharacter::Turn(float Value)
{
	if (CombatSystemCP->GetMoveType() == MOVE_TYPE::MOVE_IDLE || CombatSystemCP->GetMoveType() == MOVE_TYPE::MOVE_JUMP)
	{
		AddControllerYawInput(Value);
	}
}

void AAttackCharacter::LookUp(float Value)
{
	AddControllerPitchInput(Value);
}

void AAttackCharacter::OnJump()
{
	if (CombatSystemCP->GetMoveType() == MOVE_TYPE::MOVE_IDLE)
	{
		InputCommandCP->bJumpKeyDown = true;
		ACharacter::Jump();
	}
}

void AAttackCharacter::StopJump()
{
	InputCommandCP->bJumpKeyDown = false;
	ACharacter::StopJumping();
}

void AAttackCharacter::Test_createSection(UAnimMontage* montage)
{
	if (montage)
	{
		/*int frames = montage->GetNumberOfFrames();
		float step = frames / 30.0f;
		montage->AddAnimCompositeSection("Act1", 0.0f);
		montage->AddAnimCompositeSection("Act2", 0.1f * step);
		montage->AddAnimCompositeSection("Act3", 0.5f * step);
		montage->AddAnimCompositeSection("Act4", 0.8f * step);
		montage->DeleteAnimCompositeSection(0);*/

		//UAnimNotify_PlaySound* playSnd = CreateDefaultSubobject<UAnimNotify_PlaySound>(TEXT("playSnd"));
	}
}

void AAttackCharacter::Play_TestMontage()
{
	if (TestMtg)
	{
		GetMesh()->GetAnimInstance()->Montage_Play(TestMtg);
	}
}

void AAttackCharacter::OnOverlapBegin_Implementation(UPrimitiveComponent* Comp, 
	AActor* otherActor, UPrimitiveComponent* otherComp, int otherBodyIndex, 
	bool bFromSweep, const FHitResult& SweepResult)
{
	if (Comp->GetCollisionObjectType() == HitBoxObjectChannel && otherComp->GetCollisionObjectType() == HitBoxObjectChannel &&
		otherActor != this && HitBoxCPs.Contains(otherComp) == false)
	{
		FString DisplayStr;
		DisplayStr.Append(this->GetName()).Append("--").Append(Comp->GetName()).Append("--").Append(otherActor->GetName()).Append("--").Append(otherComp->GetName());
		GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Green, FString::Printf(TEXT("%s"), *DisplayStr));
	}
}

void AAttackCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	TArray<UActorComponent*> Comps = GetComponentsByClass(UBoxComponent::StaticClass());
	for (int i = 0; i < Comps.Num(); ++i)
	{
		UBoxComponent* HitBox = Cast<UBoxComponent>(Comps[i]);
		if (HitBox && HitBox->GetCollisionObjectType() == HitBoxObjectChannel)
		{
			HitBoxCPs.Add(HitBox);
			HitBoxNames.Add(HitBox->GetName());
		}
	}

	for (int i = 0; i < HitBoxCPs.Num(); ++i)
	{
		HitBoxCPs[i]->OnComponentBeginOverlap.AddDynamic(this, &AAttackCharacter::OnOverlapBegin);
	}
}

// Copyright IceRiver. All Rights Reserved.

#include "CombatSystemComponent.h"

#include "Animation/AnimInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Meteor/AttackCharacter.h"
#include "Meteor/Components/InputCommandComponent.h"
#include "Engine.h"

UCombatSystemComponent::UCombatSystemComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	MoveType = MOVE_TYPE::MOVE_IDLE;
	NextPoseTimer = 0.0f;
	NextPoseMtg = nullptr;

	CurrentAnimFlag = EAnimFlag::Unknow;

	ActoinStateFactory = new FMeteorActionStateFactory();

	CurrentActionState = nullptr;

	DashHaltTime = 0.2f;
	DashHaltTimer = 0.0f;
}

void UCombatSystemComponent::BeginPlay()
{
	Super::BeginPlay();

	AttackCharacter = Cast<AAttackCharacter>(GetOwner());

	CurrentActionState = ActoinStateFactory->CreateIdleState(AttackCharacter);
}

void UCombatSystemComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (CurrentActionState)
	{
		CurrentActionState->Execute(DeltaTime);
	}
}

void UCombatSystemComponent::SetInputCommandComponent(UInputCommandComponent* InputCmdCP)
{
	InputCommandCP = InputCmdCP;
	this->AddTickPrerequisiteComponent(InputCommandCP);
}

UAnimMetaData_SectionInfo* UCombatSystemComponent::GetAnimMetaData(UAnimMontage* montage)
{
	if (montage)
	{
		const TArray<UAnimMetaData*> MetaDatas = montage->GetMetaData();
		for (int i = 0; i < MetaDatas.Num(); ++i)
		{
			UAnimMetaData_SectionInfo* sectionInfo = Cast<UAnimMetaData_SectionInfo>(MetaDatas[i]);
			if (sectionInfo)
				return sectionInfo;
		}
	}
	return nullptr;
}

UAnimInstance* UCombatSystemComponent::GetAnimInstance()
{
	if (AttackCharacter)
	{
		UAnimInstance* AnimInstance = AttackCharacter->GetMesh()->GetAnimInstance();
		if (AnimInstance)
		{
			return AnimInstance;
		}
	}
	return nullptr;
}

EAnimFlag UCombatSystemComponent::GetAnimFlag(UAnimMontage* montage)
{
	if (montage)
	{
		const TArray<UAnimMetaData*> MetaDatas = montage->GetMetaData();
		for (int i = 0; i < MetaDatas.Num(); ++i)
		{
			UAnimMetaData_Flag* flag = Cast<UAnimMetaData_Flag>(MetaDatas[i]);
			if (flag)
			{
				return flag->AnimFlag;
			}
		}
	}
	return EAnimFlag::Unknow;
}

void UCombatSystemComponent::ChangeActionState(FActionState* newState)
{
	CurrentActionState->Exit();
	CurrentActionState = newState;
	CurrentActionState->Enter();
}

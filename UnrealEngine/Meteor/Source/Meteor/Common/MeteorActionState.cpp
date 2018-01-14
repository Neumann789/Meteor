
#include "Common/MeteorActionState.h"
#include "Components/InputCommandComponent.h"
#include "Components/CombatSystemComponent.h"


#include "AttackCharacter.h"
#include "Animation/AnimInstance.h"
#include "GameFramework/CharacterMovementComponent.h"


FActionState::FActionState(AAttackCharacter* owner)
	: Owner(owner)
{
	InputCommandCP = owner->GetInputCommandComponent();
	CombatSystemCP = owner->GetCombatSystemComponent();

	AnimInstance = owner->GetMesh()->GetAnimInstance();
}

FIdleState::FIdleState(AAttackCharacter* owner)
	: FActionState(owner)
{

}

void FIdleState::Enter()
{
	InputCommandCP->StateNo = -1;
	InputCommandCP->RecreateStateRecord(-1);
}

void FIdleState::Execute(float DeltaTime)
{
	// Attack
	if (InputCommandCP->StateNo != -1 && InputCommandCP->bCanAttack)
	{
		FAttackState* CombatAttack = CombatSystemCP->ActoinStateFactory->CreateAttackState(Owner);
		CombatAttack->Init(InputCommandCP->StateNo, ATTACK_ANIM_STATE::ANIM_PLYING, CombatSystemCP->NextPoseTime);

		CombatSystemCP->ChangeActionState(CombatAttack);
		InputCommandCP->bHasControl = false;
		InputCommandCP->bBeginChain = false;

		return;
	}
	else
	{
		InputCommandCP->bHasControl = true;
		InputCommandCP->bBeginChain = true;
	}

	// Jump

	// Crouch

	// DashMove
}

void FIdleState::Exit()
{
	
}

void FIdleState::Clear()
{

}

FAttackState::FAttackState(AAttackCharacter* owner)
	: FActionState(owner)
{
	PoseIndex = -1;
	PoseMtg = nullptr;
	PoseSectionInfo = nullptr;
}

void FAttackState::Init(int32 Pose, ATTACK_ANIM_STATE AnimState, float TransitTIme)
{
	PoseIndex = Pose;
	EnterAnimState = AnimState;
	AnimTransitTime = TransitTIme;

	PoseMtg = InputCommandCP->GetPoseMontage(PoseIndex);
	PoseSectionInfo = CombatSystemCP->GetAnimMetaData(PoseMtg);
}

void FAttackState::Enter()
{
	Owner->GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Falling);

	if (EnterAnimState == ATTACK_ANIM_STATE::ANIM_BEGIN)
	{
		PoseMtg->BlendIn.SetBlendTime(AnimTransitTime);
		AnimInstance->Montage_Play(PoseMtg);
		AnimInstance->Montage_JumpToSection(PoseSectionInfo->NextPoseIn, PoseMtg);
		AnimInstance->Montage_Pause(PoseMtg);
		AnimTransitTimer = 0.0f;
	}
	else if (EnterAnimState == ATTACK_ANIM_STATE::ANIM_PLYING)
	{
		AnimInstance->Montage_Stop(0.0f);
		AnimInstance->Montage_Play(PoseMtg);
	}
}

void FAttackState::Execute(float DeltaTime)
{
	UAnimMontage* ActiveAnimMtg = AnimInstance->GetCurrentActiveMontage();
	if (ActiveAnimMtg && ActiveAnimMtg == PoseMtg)
	{
		// Combat 过渡
		if (EnterAnimState == ATTACK_ANIM_STATE::ANIM_BEGIN)
		{
			AnimTransitTimer += DeltaTime;
			if (AnimTransitTimer > AnimTransitTime)
			{
				AnimInstance->Montage_Resume(PoseMtg);
				EnterAnimState = ATTACK_ANIM_STATE::ANIM_PLYING;
			}
		}

		// 正常的Playing
		if (EnterAnimState == ATTACK_ANIM_STATE::ANIM_PLYING)
		{
			FName sectionName = AnimInstance->Montage_GetCurrentSection(PoseMtg);
			if (PoseSectionInfo)
			{
				if (sectionName.IsEqual(PoseSectionInfo->NextPoseOut))
				{
					InputCommandCP->bBeginChain = true; // 有一帧的滞后
					if (InputCommandCP->bHasControl && InputCommandCP->bCanAttack)
					{
						InputCommandCP->bBeginChain = false;
						InputCommandCP->bHasControl = false;

						FAttackState* CombatAttack = CombatSystemCP->ActoinStateFactory->CreateAttackState(Owner);
						CombatAttack->Init(InputCommandCP->StateNo, ATTACK_ANIM_STATE::ANIM_BEGIN, CombatSystemCP->NextPoseTime);
						CombatSystemCP->ChangeActionState(CombatAttack);
						return;
					}
				}

				float* ratio = PoseSectionInfo->SectionSpeeds.Find(sectionName);
				if (ratio)
				
					AnimInstance->Montage_SetPlayRate(ActiveAnimMtg, *ratio);
				else
					AnimInstance->Montage_SetPlayRate(ActiveAnimMtg, 1.0f);
			}
		}

		// HitPause

		// 
	}
	else
	{
		FIdleState* idleState = CombatSystemCP->ActoinStateFactory->CreateIdleState(Owner);
		CombatSystemCP->ChangeActionState(idleState);
	}
}

void FAttackState::Exit()
{
	
}

void FAttackState::Clear()
{
	PoseIndex = -1;
	PoseMtg = nullptr;
	PoseSectionInfo = nullptr;
}

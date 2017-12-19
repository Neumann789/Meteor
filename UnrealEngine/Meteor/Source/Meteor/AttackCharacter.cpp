
#include "AttackCharacter.h"
#include "Meteor.h"
#include "Animation/AnimInstance.h"
#include "MyAnimMetaData.h"
#include "Engine.h"

AAttackCharacter::AAttackCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	CurrentPoseIndex = -1;

	bAcceptInput = false;

	NextPoseTime = 0.2f;

	AttackState = ATTACK_STATE::ATK_IDLE;

	bAttackKeyDown = false;

	NextPoseTimer = 0.0f;

	NextPoseMtg = nullptr;

	MAX_INPUT_BUFFER = 4;
}


void AAttackCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	StreamMgr.LoadSynchronous(PoseInfoTable);
	StreamMgr.LoadSynchronous(Dao_PoseChangeTable);

	if (PoseInfoTable)
	{
		PoseInfoTable->GetAllRows(this->GetName(), Dao_AllPoses);
	}
}

void AAttackCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance == nullptr)
		return;

	switch (AttackState)
	{
	case AAttackCharacter::ATK_IDLE:
	{
		if (bAttackKeyDown)
		{
			const TArray<FString> AllCmdKeys = GetAllCominationKey();
			CurrentPoseIndex = GetNextPose(-1, AllCmdKeys);
			ConsumeInputKey();
			
			bAcceptInput = false;
			NextPoseTime = 0.2f;
			NextPoseTimer = 0.0f;
			NextPoseMtg = nullptr;

			if (CurrentPoseIndex != -1)
			{
				UAnimMontage* poseMtg = GetPoseMontage(CurrentPoseIndex);
				if (poseMtg)
				{
					GetAnimMetaData(poseMtg);
					AnimInstance->Montage_Play(poseMtg);
					AttackState = ATK_PLAYING;
					GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Falling);
				}
			}
		}
		break;
	}
	case AAttackCharacter::ATK_PLAYING:
	{
		UAnimMontage* ActiveMontage = AnimInstance->GetCurrentActiveMontage();
		if (ActiveMontage)
		{
			FName sectionName = AnimInstance->Montage_GetCurrentSection(ActiveMontage);

			// 设置playRate
			float* ratio = SectionRatioCache.Find(sectionName);
			if (ratio)
			{
				//UE_LOG(LogTemp, Warning, TEXT("%s %s: play rate %f"), *(ActiveMontage->GetName()), *(sectionName.ToString()), *ratio);
				AnimInstance->Montage_SetPlayRate(ActiveMontage, *ratio);
			}
			
			// 检测是否需要跳转
			if (sectionName.IsEqual(NextPoseOut) && bAttackKeyDown)
			{
				const TArray<FString> AllCmdKeys = GetAllCominationKey();

				CurrentPoseIndex = GetNextPose(CurrentPoseIndex, AllCmdKeys);
				ConsumeInputKey();

				if (CurrentPoseIndex != -1)
				{
					UAnimMontage* poseMtg = GetPoseMontage(CurrentPoseIndex);
					if (poseMtg)
					{
						GetAnimMetaData(poseMtg);

						// 执行实际的过渡，主要使用的功能是，当一个Montage过渡到下一个Montage时，会直接从当前位置过渡到下一个位置，不过会出现blend
						tmpBlend = poseMtg->BlendIn;
						poseMtg->BlendIn.SetBlendTime(NextPoseTime);
						AnimInstance->Montage_Play(poseMtg);
						AnimInstance->Montage_JumpToSection(NextPoseIn, poseMtg);
						AnimInstance->Montage_Pause(poseMtg);

						NextPoseMtg = poseMtg;
						NextPoseTimer = 0.0f;

						AttackState = ATK_NEXTPOSE;
					}
				}
			}
		}
		else// 如果当前动画已经播完，则进入Idle状态
		{
			AttackState = ATK_IDLE;
			CurrentPoseIndex = -1;
			GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Falling);
		}
		break;
	}
	case AAttackCharacter::ATK_NEXTPOSE:
	{
		NextPoseTimer += DeltaTime;

		if (NextPoseTimer > NextPoseTime)
		{
			AnimInstance->Montage_Resume(NextPoseMtg);
			AttackState = ATK_PLAYING;
			GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Falling);
		}
		break;
	}
	default:
		break;
	}
}


void AAttackCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	//Super::SetupPlayerInputComponent(PlayerInputComponent);

	check(PlayerInputComponent);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAction("Attack", IE_Pressed, this, &AAttackCharacter::OnAttack);
	PlayerInputComponent->BindAction("Attack", IE_Released, this, &AAttackCharacter::StopAttack);

	PlayerInputComponent->BindAction("Left", IE_Pressed, this, &AAttackCharacter::OnLeftKeyDown);
	PlayerInputComponent->BindAction("Right", IE_Pressed, this, &AAttackCharacter::OnRightKeyDown);
	PlayerInputComponent->BindAction("Up", IE_Pressed, this, &AAttackCharacter::OnUpKeyDown);
	PlayerInputComponent->BindAction("Down", IE_Pressed, this, &AAttackCharacter::OnDownKeyDown);

	PlayerInputComponent->BindAxis("MoveForward", this, &AAttackCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AAttackCharacter::MoveRight);

	PlayerInputComponent->BindAxis("Turn", this, &AAttackCharacter::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &AAttackCharacter::LookUp);
}


void AAttackCharacter::MoveForward(float Value)
{
	AddMovementInput(GetActorForwardVector(), Value);
}

void AAttackCharacter::MoveRight(float Value)
{
	AddMovementInput(GetActorRightVector(), Value);
}

void AAttackCharacter::Turn(float Value)
{
	AddControllerYawInput(Value);
}

void AAttackCharacter::LookUp(float Value)
{
	AddControllerPitchInput(Value);
}

void AAttackCharacter::OnAttack()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance)
	{
		UAnimMontage* activeMontage = AnimInstance->GetCurrentActiveMontage();
		if (activeMontage && bAcceptInput)
		{
			bAttackKeyDown = true;
			PushAttackKey(ATTACK_KEY::KEY_A, GetWorld()->GetTimeSeconds());
		}
		else if (activeMontage == nullptr)
		{
			bAttackKeyDown = true;
			PushAttackKey(ATTACK_KEY::KEY_A, GetWorld()->GetTimeSeconds());
		}
	}
}

void AAttackCharacter::StopAttack()
{
	
}

void AAttackCharacter::OnLeftKeyDown()
{
	PushAttackKey(ATTACK_KEY::KEY_L, GetWorld()->GetTimeSeconds());
}

void AAttackCharacter::OnRightKeyDown()
{
	PushAttackKey(ATTACK_KEY::KEY_R, GetWorld()->GetTimeSeconds());
}

void AAttackCharacter::OnUpKeyDown()
{
	PushAttackKey(ATTACK_KEY::KEY_U, GetWorld()->GetTimeSeconds());
}

void AAttackCharacter::OnDownKeyDown()
{
	PushAttackKey(ATTACK_KEY::KEY_D, GetWorld()->GetTimeSeconds());
}

UAnimMontage* AAttackCharacter::GetPoseMontage(int32 poseIndex)
{
	FString poseStr = FString::FromInt(poseIndex);

	if (PoseInfoTable)
	{
		FPoseInputTable* PoseInfo = PoseInfoTable->FindRow<FPoseInputTable>(FName(*poseStr), "");
		if (PoseInfo)
		{
			UAnimMontage* poseMtg = PoseInfo->PoseMontage.Get();
			if (poseMtg == nullptr)
			{
				poseMtg = StreamMgr.LoadSynchronous(PoseInfo->PoseMontage);

				//poseMtg = PoseInfo->PoseMontage.Get();
				if (poseMtg)
				{
					GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red, FString::Printf(TEXT("Load %s succeed!"), *(poseMtg->GetName())));
				}
				else
				{
					GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red, FString::Printf(TEXT("Load  %s error!"), *poseStr));
				}
			}
			return poseMtg;
		}
	}
	return nullptr;
}

void AAttackCharacter::GetAnimMetaData(UAnimMontage* montage)
{
	if (montage)
	{
		SectionRatioCache.Empty(4);
		SectionRatioHasSet.Empty(4);
		const TArray<UAnimMetaData*> MetaDatas = montage->GetMetaData();
		for (int i = 0; i < MetaDatas.Num(); ++i)
		{
			UAnimMetaData_SectionInfo* sectionInfo = Cast<UAnimMetaData_SectionInfo>(MetaDatas[i]);
			if (sectionInfo)
			{
				SectionRatioCache = sectionInfo->SectionSpeeds;
				NextPoseIn = sectionInfo->NextPoseIn;
				NextPoseOut = sectionInfo->NextPoseOut;
				break;
			}
		}
	}
}

bool AAttackCharacter::ConsumeInputKey()
{
	bool lastInput = bAttackKeyDown;
	bAttackKeyDown = false;

	// Test Input
	TArray<FString> PosiableComKeys = GetPosiableCombinationKey();
	FString PosiableComKeysCmd;
	for (int32 i = 0; i < PosiableComKeys.Num(); ++i)
	{
		PosiableComKeysCmd.Append(PosiableComKeys[i]);
		PosiableComKeysCmd.Append("-");
	}
	GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, FString::Printf(TEXT("%s %f"), *PosiableComKeysCmd, GetWorld()->GetTimeSeconds()));

	// Test combInput
	TArray<FString> AllCmdKeys = GetAllCominationKey();
	FString AllCombKeysCmd;
	for (int i = 0; i < AllCmdKeys.Num(); ++i)
	{
		AllCombKeysCmd.Append(AllCmdKeys[i]).Append("\n");
	}
	GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Blue, FString::Printf(TEXT("%s"), *AllCombKeysCmd));

	InputBuffer.Empty(4);

	return lastInput;
}

TArray<FString> AAttackCharacter::GetPosiableCombinationKey()
{
	TArray<FString> ComGroups;
	int32 bufferNum = InputBuffer.Num();
	for (int32 i = 0; i < InputBuffer.Num(); ++i)
	{
		FrameInputKey inputKey = InputBuffer[i];

		FString CombKeyName;

		if (inputKey.GetValidStr(CombKeyName))
		{
			ComGroups.Add(CombKeyName);
		}
	}
	return ComGroups;
}

TArray<FString> AAttackCharacter::GetAllCominationKey()
{
	const int32 bufferNum = InputBuffer.Num();
	TArray<FString> ComGroups;
	if (bufferNum > 0)
	{
		// 只要添加进来的保证都是有效的
		TArray<TArray<TCHAR>> ValidChars;
		for (int32 i = 0; i < bufferNum; ++i)
		{
			FrameInputKey inputKey = InputBuffer[i];

			TArray<TCHAR> chars;

			if (inputKey.GetValidChars(chars))
			{
				ValidChars.Add(chars);
			}
		}

		// i = 0, bufferNum 个组合键生成的输入命令
		// i = 1, bufferNum-1个组合键生成的输入命令
		TArray<TArray<FString>> LayrsCmds;
		LayrsCmds.SetNum(bufferNum);
		
		// 先计算最后一个输入
		TArray<TCHAR> lastInputChars = ValidChars[bufferNum-1];
		TArray<FString> lastInputCmds;
		for (int c = 0; c < lastInputChars.Num(); ++c)
		{
			FString cmd;
			cmd.AppendChar(lastInputChars[c]);
			lastInputCmds.Add(cmd);
			LayrsCmds[bufferNum - 1] = lastInputCmds;
		}

		// 倒着生成 即倒数第几个生成的
		for (int i = bufferNum - 2; i >= 0; --i)
		{
			const TArray<FString>& LastInputs = LayrsCmds[i + 1];
			TArray<FString> CurrInputs;
			TArray<TCHAR> CurrInputChars = ValidChars[i];
			
			for (int c = 0; c < CurrInputChars.Num(); ++c)
			{
				FString cmd;
				cmd.AppendChar(CurrInputChars[c]).AppendChar(TCHAR('-'));
				for (int lastIdx = 0; lastIdx < LastInputs.Num(); ++lastIdx)
				{
					cmd += LastInputs[lastIdx];
					CurrInputs.Add(cmd);
				}
			}
			LayrsCmds[i] = CurrInputs;
		}

		// 输出所有可能的组合
		for (int i = 0; i < LayrsCmds.Num(); ++i)
		{
			ComGroups.Append(LayrsCmds[i]);
		}
	}
	return ComGroups;
}

void AAttackCharacter::PushAttackKey(ATTACK_KEY key, float time)
{
	if (InputBuffer.Num() == 0)
	{
		FrameInputKey inputKey(key, time);
		InputBuffer.Add(inputKey);
		return;
	}
	else
	{
		// 对 ATK_A键进行特殊判断
		if (key == KEY_A)
		{
			bool bHasClickA = false;
			for (int i = 0; i < InputBuffer.Num(); ++i)
			{
				if (InputBuffer[i].A_Down)
				{
					bHasClickA = true;
				}
			}
			if (bHasClickA)
			{
				InputBuffer.Empty(4);
				FrameInputKey inputKey(key, time);
				InputBuffer.Add(inputKey);
				return;
			}
		}

		FrameInputKey& inputKey = InputBuffer.Last();
		if (inputKey.timeStamp == time)
		{
			inputKey.SetKeyDown(key);
		}
		else if (inputKey.timeStamp < time)
		{
			FrameInputKey inputKey(key, time);
			InputBuffer.Add(inputKey);

			while (InputBuffer.Num() > MAX_INPUT_BUFFER)
			{
				InputBuffer.RemoveAt(0);
			}
		}
	}
}

// inputCmds 中排在前面的cmd优先可能被触发
int32 AAttackCharacter::GetNextPose(int32 poseIdx, const TArray<FString>& inputCmds, bool bInAir)
{
	if (PoseInfoTable == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Cyan, FString::Printf(TEXT("Reload  %s!"), *PoseInfoTable->GetName()));
		StreamMgr.LoadSynchronous(PoseInfoTable);
	}
	if (Dao_PoseChangeTable)
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Cyan, FString::Printf(TEXT("Reload  %s!"), *Dao_PoseChangeTable->GetName()));
		StreamMgr.LoadSynchronous(Dao_PoseChangeTable);
	}
	if (PoseInfoTable && Dao_PoseChangeTable && inputCmds.Num() > 0)
	{
		FString poseStr = FString::FromInt(poseIdx);
		FPoseChangeTable* poseChange = Dao_PoseChangeTable->FindRow<FPoseChangeTable>(FName(*poseStr), "");
		if (poseChange)
		{
			const TArray<int32>& NextPoses = poseChange->NextPoses;

			for (int32 c = 0; c < inputCmds.Num(); ++c)
			{
				FName inputCmd = FName(*inputCmds[c]);
				for (int32 i = 0; i < NextPoses.Num(); ++i)
				{
					FString tmpPoseStr = FString::FromInt(NextPoses[i]);
					const FPoseInputTable* PoseInfo = PoseInfoTable->FindRow<FPoseInputTable>(FName(*tmpPoseStr), "");
					if (PoseInfo)
					{
						FName poseCmd = FName(*PoseInfo->PoseInputKey);
						if (poseCmd.Compare(inputCmd) == 0)
						{
							return NextPoses[i];
						}
					}
				}
			}
		}
	}
	return -1;
}

AAttackCharacter::FrameInputKey::FrameInputKey()
{
	Reset();
}

AAttackCharacter::FrameInputKey::FrameInputKey(ATTACK_KEY key, float time)
{
	Reset();
	timeStamp = time;
	SetKeyDown(key);
}

void AAttackCharacter::FrameInputKey::SetKeyDown(ATTACK_KEY key)
{
	switch (key)
	{
	case AAttackCharacter::KEY_L:
		L_Down = true;
		break;
	case AAttackCharacter::KEY_R:
		R_Down = true;
		break;
	case AAttackCharacter::KEY_U:
		U_Down = true;
		break;
	case AAttackCharacter::KEY_D:
		D_Down = true;
		break;
	case AAttackCharacter::KEY_A:
		A_Down = true;
		break;
	case AAttackCharacter::KEY_NUM:
		break;
	default:
		break;
	}
}

bool AAttackCharacter::FrameInputKey::GetValidChars(TArray<TCHAR>& chars) const
{
	if (timeStamp < 0.0f || (!L_Down && !R_Down && !U_Down && !D_Down && !A_Down))
	{
		return false;
	}
	if (L_Down)
	{
		chars.Add(TCHAR('L'));
	}
	if (R_Down)
	{
		chars.Add(TCHAR('R'));
	}
	if (U_Down)
	{
		chars.Add(TCHAR('U'));
	}
	if (D_Down)
	{
		chars.Add(TCHAR('D'));
	}
	if (A_Down)
	{
		chars.Add(TCHAR('A'));
	}
	return true;
}

bool AAttackCharacter::FrameInputKey::GetValidStr(FString& str) const
{
	TArray<TCHAR> validChars;
	if (GetValidChars(validChars))
	{
		for (int32 i = 0; i < validChars.Num(); ++i)
		{
			str.AppendChar(validChars[i]);
		}
		return true;
	}
	return false;
}

void AAttackCharacter::FrameInputKey::Reset()
{
	timeStamp = -1.0f;
	L_Down = false;
	R_Down = false;
	U_Down = false;
	D_Down = false;
	A_Down = false;
}

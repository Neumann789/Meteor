// Copyright IceRiver. All Rights Reserved.

#include "InputBufferComponent.h"
#include "Engine.h"

UInputBufferComponent::UInputBufferComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	MaxInputBufferCount = 3;
	
	MaxIdleHoldTime = 0.3f;

	bAttackKeyDown = false;
}


void UInputBufferComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UInputBufferComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

}

void UInputBufferComponent::PushAttackKey(ATTACK_KEY key, float time)
{
	if (InputBuffer.Num() == 0)
	{
		FrameInputKey inputKey(key, time);
		InputBuffer.Add(inputKey);
	}
	else
	{
		FrameInputKey& inputKey = InputBuffer.Last();
		if (inputKey.timeStamp == time)
		{
			inputKey.SetKeyDown(key);
		}
		else if (inputKey.timeStamp < time)
		{
			FrameInputKey inputKey(key, time);
			InputBuffer.Add(inputKey);
		}
	}
}

void UInputBufferComponent::DebugInputBuffer()
{
	if (InputBuffer.Num() > 0 && bAttackKeyDown)
	{
		FString InputBufferStr;
		for (int32 i = 0; i < InputBuffer.Num(); ++i)
		{
			FrameInputKey inputKey = InputBuffer[i];
			FString KeyStr;
			if (inputKey.GetValidStr(KeyStr))
			{
				InputBufferStr.Append(KeyStr).Append("-");
			}
		}
		GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red, FString::Printf(TEXT("InputBuffer: %s"), *InputBufferStr));
	}
}

TArray<FString> UInputBufferComponent::GetInputCommonds() const
{
	const int32 bufferNum = InputBuffer.Num();
	TArray<FString> InputCommonds;
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
		TArray<TCHAR> lastInputChars = ValidChars[bufferNum - 1];
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
				cmd.AppendChar(CurrInputChars[c]).AppendChar(TCHAR(','));
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
			InputCommonds.Append(LayrsCmds[i]);
		}
	}
	return InputCommonds;
}

TArray<FString> UInputBufferComponent::GetPosiableCombinationKey()
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

void UInputBufferComponent::UpdateInputBuffer()
{
	float time = GetWorld()->GetTimeSeconds();
	// 先将多余的键去除掉
	while (InputBuffer.Num() > MaxInputBufferCount)
	{
		InputBuffer.RemoveAt(0);
	}
	
	// 处理超时
	while (InputBuffer.Num())
	{
		if (InputBuffer[0].timeStamp + MaxIdleHoldTime < time)
		{
			InputBuffer.RemoveAt(0);
		}
		else
		{
			break;
		}
	}
	// 移动状态时，应该保留超时的一个数据
	// 在没有攻击键的时候，已经超时，怎么处理
}

bool UInputBufferComponent::ConsumeInputKey()
{
	bool lastInput = bAttackKeyDown;
	bAttackKeyDown = false;
	InputBuffer.Empty(MaxInputBufferCount);
	return lastInput;
}

void UInputBufferComponent::OnUpKeyDown()
{
	PushAttackKey(ATTACK_KEY::KEY_U, GetWorld()->GetTimeSeconds());
}

void UInputBufferComponent::OnUpKeyRelease()
{
	ReleaseAttackKey(ATTACK_KEY::KEY_U, GetWorld()->GetTimeSeconds());
}

void UInputBufferComponent::OnDownKeyDown()
{
	PushAttackKey(ATTACK_KEY::KEY_D, GetWorld()->GetTimeSeconds());
}

void UInputBufferComponent::OnDownKeyRelease()
{
	ReleaseAttackKey(ATTACK_KEY::KEY_D, GetWorld()->GetTimeSeconds());
}

void UInputBufferComponent::OnRightKeyDown()
{
	PushAttackKey(ATTACK_KEY::KEY_R, GetWorld()->GetTimeSeconds());
}

void UInputBufferComponent::OnRightKeyRelease()
{
	ReleaseAttackKey(ATTACK_KEY::KEY_R, GetWorld()->GetTimeSeconds());
}

void UInputBufferComponent::OnLeftKeyDown()
{
	PushAttackKey(ATTACK_KEY::KEY_L, GetWorld()->GetTimeSeconds());
}

void UInputBufferComponent::OnLeftKeyRelease()
{
	ReleaseAttackKey(ATTACK_KEY::KEY_L, GetWorld()->GetTimeSeconds());
}

void UInputBufferComponent::PressAttackKey(ATTACK_KEY key, float time)
{
	// 同一帧，同时按下多个按键，暂时只记录优先级最高的按键

	// 从逻辑的角度来说，应该看做是两次按键，比如同时按下L-R，这时

	// 先测下速度再说

	// 因为同一个按键不可能还没有释放，就再次按下，随意直接记录即可
	//PushKeyRecord record(key, time);
	//PushKeysBuffer.Add(record);
}

void UInputBufferComponent::ReleaseAttackKey(ATTACK_KEY key, float time)
{
	// 释放时，同样需要考虑，同一帧多次按下按键
	
	if (PushKeysBuffer.Num() > 0)
	{
		// 倒着查，离得最近的键就是刚才按下的键
		for (int i = PushKeysBuffer.Num() - 1; i >= 0; --i)
		{
			PushKeyRecord& Record = PushKeysBuffer[i];
			if (Record.Key == key)
			{
				if (Record.StartTimeStamp < time && Record.EndTimeStamp == -1.0f)
				{
					Record.EndTimeStamp = time;
				}
				else
				{
					GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red, FString::Printf(TEXT("Invalid Release Key: %n %f"), (int)key, time));
				}
				return;
			}
		}
	}
}

TArray<FString> UInputBufferComponent::GenerateAttackCommond() const
{
	TArray<FString> AttackCmds;

	return AttackCmds;
}

UInputBufferComponent::FrameInputKey::FrameInputKey()
{
	Reset();
}

UInputBufferComponent::FrameInputKey::FrameInputKey(ATTACK_KEY key, float time)
{
	Reset();
	timeStamp = time;
	SetKeyDown(key);
}

void UInputBufferComponent::FrameInputKey::SetKeyDown(ATTACK_KEY key)
{
	switch (key)
	{
	case ATTACK_KEY::KEY_L:
		L_Down = true;
		break;
	case ATTACK_KEY::KEY_R:
		R_Down = true;
		break;
	case ATTACK_KEY::KEY_U:
		U_Down = true;
		break;
	case ATTACK_KEY::KEY_D:
		D_Down = true;
		break;
	case ATTACK_KEY::KEY_NUM:
		break;
	default:
		break;
	}
}

bool UInputBufferComponent::FrameInputKey::GetValidChars(TArray<TCHAR>& chars) const
{
	if (timeStamp < 0.0f || (!L_Down && !R_Down && !U_Down && !D_Down))
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
	return true;
}

bool UInputBufferComponent::FrameInputKey::GetValidStr(FString& str) const
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

void UInputBufferComponent::FrameInputKey::Reset()
{
	timeStamp = -1.0f;
	L_Down = false;
	R_Down = false;
	U_Down = false;
	D_Down = false;
}


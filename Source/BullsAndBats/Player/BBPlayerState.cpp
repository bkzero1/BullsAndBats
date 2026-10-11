// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/BBPlayerState.h"

#include "Net/UnrealNetwork.h"

void ABBPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Replicated로 선언한 현재 횟수를 실제 복제 대상 목록에 등록합니다.
	DOREPLIFETIME(ThisClass, CurrentGuessCount);
}

bool ABBPlayerState::TryIncreaseGuessCount()
{
	// 클라이언트의 직접 변경과 최대 횟수 초과를 막습니다.
	if (HasAuthority() == false || CurrentGuessCount >= MaxGuessCount)
	{
		return false;
	}

	++CurrentGuessCount;
	return true;
}

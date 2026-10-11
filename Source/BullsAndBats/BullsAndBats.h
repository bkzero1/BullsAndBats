// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "GameFramework/Actor.h"

// 강의처럼 프로젝트 헤더에 공용 출력 함수의 선언과 구현을 함께 둡니다.
class BBFunctionLibrary
{
public:
	static void MyPrintString(const AActor* InWorldContextActor, const FString& InString, float InTimeToDisplay = 1.f, FColor InColor = FColor::Cyan)
	{
		if (IsValid(GEngine) == true && IsValid(InWorldContextActor) == true)
		{
			// 강의와 동일하게 클라이언트와 리슨 서버는 화면에, 나머지는 로그에 출력합니다.
			if (InWorldContextActor->GetNetMode() == NM_Client || InWorldContextActor->GetNetMode() == NM_ListenServer)
			{
				GEngine->AddOnScreenDebugMessage(-1, InTimeToDisplay, InColor, InString);
			}
			else
			{
				UE_LOG(LogTemp, Log, TEXT("%s"), *InString);
			}
		}
	}
};

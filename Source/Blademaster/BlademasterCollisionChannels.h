#pragma once

#include "Engine/EngineTypes.h"

namespace BlademasterCollisionChannels
{
	// DefaultEngine.ini의 [/Script/Engine.CollisionProfile]에 이름을 "Weapon"으로 등록해둔
	// 트레이스 채널(ECC_GameTraceChannel1)이다. 슬롯 번호만으로는 무슨 채널인지 알 수 없어서
	// 여기에 의미 있는 이름을 붙여둔다.
	constexpr ECollisionChannel Weapon = ECC_GameTraceChannel1;
}

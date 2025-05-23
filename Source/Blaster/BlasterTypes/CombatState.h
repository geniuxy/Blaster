﻿#pragma once

UENUM(BlueprintType)
enum class ECombatState : uint8
{
	ECS_Unoccupied UMETA(DisplayName = "Unoccupied"),
	ECS_Reloading UMETA(DisplayName = "Reloading"),
	ECS_ThrowingGrenade UMETA(DisplayName = "Throwing Grenade"),
	ECS_SwapingWeapons UMETA(DisplayName = "Swaping Weapons"),

	ECS_DefaultMax UMETA(DisplayName = "DefaultMax")
};

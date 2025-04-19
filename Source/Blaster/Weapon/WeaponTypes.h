#pragma once

#define TRACE_LENGTH 80000.f

#define CUSTOM_DEPTH_PURPLE 250
#define CUSTOM_DEPTH_BLUE 251
#define CUSTOM_DEPTH_TAN 252

UENUM(BlueprintType)
enum class EWeaponType:uint8
{
	EWT_AssaultRifle UMETA(DisplayName = "Assault Rifle"),
	EWT_RocketLauncher UMETA(DisplayName = "Rocket Launcher"),
	EWT_Pistol UMETA(DisplayName = "Pistol"),
	EWT_SubmachineGun UMETA(DisplayName = "Submachine Gun"),
	EWT_Shotgun UMETA(DisplayName = "Shotgun"),
	EWT_SniperRifle UMETA(DisplayName = "Sniper Rifle"),
	EWT_GrenadeLauncher UMETA(DisplayName = "Grenade Launcher"),

	EWS_Max UMETA(DisplayName = "DefaultMax")
};

UENUM(BlueprintType)
enum class EWeaponState:uint8
{
	EWS_Initial UMETA(DisplayName = "Initial State"),
	EWS_Equipped UMETA(DisplayName = "Equipped State"),
	EWS_EquippedSecondary UMETA(DisplayName = "Equipped Secondary State"),
	EWS_Dropped UMETA(DisplayName = "Dropped State"),
	EWS_Max UMETA(DisplayName = "Default Max State")
};

UENUM(BlueprintType)
enum class EFireType:uint8
{
	EFT_ProjectileWeapon UMETA(DisplayName = "Projectile Weapon"),
	EFT_HitScanWeapon UMETA(DisplayName = "Hit Scan Weapon"),
	EFT_Shotgun UMETA(DisplayName = "Shotgun"),

	EFT_Max UMETA(DisplayName = "DefaultMax")
};

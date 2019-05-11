// Fill out your copyright notice in the Description page of Project Settings.


#include "Tile.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"
#include "ActorPool.h"
#include "AI/NavigationSystemBase.h"

// Sets default values
ATile::ATile()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	NavigationBoundsOffset = FVector( 2000, 0, 0 );

	MinExtent = FVector( 0, -2000, 0 );
	MaxExtent = FVector( 4000, 2000, 0 );
}


void ATile::SetPool( UActorPool* InPool )
{
	UE_LOG( LogTemp, Warning, TEXT( "[%s] Setting Pool %s" ), *( this->GetName() ), *( InPool->GetName() ) )
	Pool = InPool;

	PositionNavMeshBoundsVolume();
}

void ATile::PositionNavMeshBoundsVolume()
{
	NavMeshBoundsVolume = Pool->Checkout();
	if( NavMeshBoundsVolume == nullptr )
	{
		UE_LOG( LogTemp, Error, TEXT( "[%s]Not enough actors in pool." ), *GetName() );
		return;
	}

	UE_LOG( LogTemp, Error, TEXT( "[%s] Checked out {%s}." ), *GetName(), *NavMeshBoundsVolume->GetName() );
	NavMeshBoundsVolume->SetActorLocation( GetActorLocation() + NavigationBoundsOffset );
	
	FNavigationSystem::Build( *GetWorld() );
}

void ATile::PlaceActors( TSubclassOf<AActor> ToSpawn, int MinSpawn, int MaxSpawn, float Radius, float MinScale, float MaxScale )
{

	TArray<FSpawnPosition> SpawnPositions = RandomSpawnPositions( MinSpawn, MaxSpawn, MinScale, MaxScale, Radius );

	for( FSpawnPosition SpawnPosition : SpawnPositions )
	{
		PlaceActor( ToSpawn, SpawnPosition );
	}
}

void ATile::PlaceAIPawns( TSubclassOf<APawn> ToSpawn, int MinSpawn, int MaxSpawn, float Radius )
{
	TArray<FSpawnPosition> SpawnPositions = RandomSpawnPositions( MinSpawn, MaxSpawn, Radius, 1, 1 );

	for( FSpawnPosition SpawnPosition : SpawnPositions )
	{
		PlaceAIPawn( ToSpawn, SpawnPosition );
	}
}

TArray<FSpawnPosition> ATile::RandomSpawnPositions( int MinSpawn, int MaxSpawn, float MinScale, float MaxScale, float Radius )
{
	TArray<FSpawnPosition> SpawnPositions;

	int NumberToSpawn = FMath::RandRange( MinSpawn, MaxSpawn );

	for( size_t i = 0; i < NumberToSpawn; i++ )
	{
		FSpawnPosition SpawnPosition;
		SpawnPosition.Scale = FMath::RandRange( MinScale, MaxScale );
		bool found = FindEmptyLocation( SpawnPosition.Location, Radius * SpawnPosition.Scale );
		if( found )
		{
			SpawnPosition.Rotation = FMath::RandRange( -180.f, 180.f );
			SpawnPositions.Add( SpawnPosition );
		}
	}

	return SpawnPositions;
}

bool ATile::FindEmptyLocation( FVector& OutLocation, float Radius )
{
	FBox Bounds( MinExtent, MaxExtent );

	bool castSucess = false;

	const int MAX_ATTEMPTS = 100;

	for( size_t i = 0; i < MAX_ATTEMPTS; i++ )
	{
		FVector CandidatePoint = FMath::RandPointInBox( Bounds );
		if( CanSpawnAtLocation( CandidatePoint, Radius ) )
		{
			OutLocation = CandidatePoint;
			return true;
		}		
	}

	return false;
}

void ATile::PlaceActor( TSubclassOf<AActor> ToSpawn, FSpawnPosition SpawnPosition )
{
	AActor* Spawned = GetWorld()->SpawnActor(ToSpawn );
	Spawned->SetActorLocation( SpawnPosition.Location );
	Spawned->SetActorRotation( FRotator( 0, SpawnPosition.Rotation, 0 ) );
	Spawned->SetActorScale3D( FVector( SpawnPosition.Scale ) );
	Spawned->AttachToActor( this, FAttachmentTransformRules( EAttachmentRule::KeepRelative, false ) );
}

void ATile::PlaceAIPawn( TSubclassOf<APawn> ToSpawn, FSpawnPosition SpawnPosition )
{
	APawn* Spawned = GetWorld()->SpawnActor<APawn>( ToSpawn );
	Spawned->SpawnDefaultController();
	Spawned->Tags.Add( "Enemy" );
	Spawned->SetActorLocation( SpawnPosition.Location );
	Spawned->SetActorRotation( FRotator( 0, SpawnPosition.Rotation, 0 ) );
	Spawned->AttachToActor( this, FAttachmentTransformRules( EAttachmentRule::KeepRelative, false ) );
}

// Called when the game starts or when spawned
void ATile::BeginPlay()
{
	Super::BeginPlay();
}

void ATile::EndPlay( const EEndPlayReason::Type EndPlayReason )
{	
	Super::EndPlay( EndPlayReason );

	Pool->Return( NavMeshBoundsVolume );
}

// Called every frame
void ATile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

bool ATile::CanSpawnAtLocation( FVector Location, float Radius )
{
	FHitResult HitResult;

	FVector GlobalLocation = ActorToWorld().TransformPosition( Location );

	FVector FinalLocation = GlobalLocation + FVector( 0, 0, 1 );

	bool HasHit = GetWorld()->SweepSingleByChannel(	HitResult,
													GlobalLocation,
													FinalLocation, //GlobalLocation,
													FQuat::Identity,
													ECollisionChannel::ECC_GameTraceChannel2,
													FCollisionShape::MakeSphere( Radius ) );

	FColor ResultColor = HasHit ? FColor::Red : FColor::Green;
	//DrawDebugCapsule( GetWorld(), GlobalLocation, 0, Radius, FQuat::Identity, ResultColor, true, 100 );

	return !HasHit;
}
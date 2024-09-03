#include "pch.h"
#include "Collectable.h"

Collectable::Collectable()
{
}

Collectable::~Collectable()
{
}

void Collectable::Initialise(ID3D11Device* device, Terrain* terrain, int id, int totalCollectables)
{
	m_collectableSphere.InitializeSphere(device);
	m_radius = 25.0f;
	m_terrain = terrain;
	m_isHit = false;
	m_id = id;
	m_totalCollectables = totalCollectables;

	SetSpawnPosition();
}

void Collectable::SetSpawnPosition()
{	
	// Limit each collectable to its own range of indices to sample from the terrain map to (mostly) prevent overlaps
	int totalPoints = m_terrain->GetWidth() * m_terrain->GetHeight();
	int pointsPerCollectable = totalPoints / m_totalCollectables;

	int index = NewRandomInt(0 + pointsPerCollectable * m_id, (m_id + 1) * pointsPerCollectable - 1);
	
	// Get random terrain point
	SimpleMath::Vector3 spawnPosition = m_terrain->GetHeightMapPoint(index);

	// Shift up in y
	m_spawnPosition = spawnPosition + SimpleMath::Vector3(0, m_radius * 2.0, 0);	
}

int Collectable::NewRandomInt(int min, int max)
{
	std::random_device device;
	std::mt19937 generator(device());
	std::uniform_int_distribution<int> distribution(min, max);	

	return distribution(generator);
}

void Collectable::CheckCollision(SimpleMath::Vector3 targetSphereCenter, float targetSphereRadius, float scalingFactor)
{
	// Calculate squared distance between centers
	SimpleMath::Vector3 d = m_spawnPosition * scalingFactor - targetSphereCenter;
	float dist2 = d.Dot(d);
	// Spheres intersect if squared distance is less than squared sum of radii
	float radiusSum = m_radius * scalingFactor + targetSphereRadius;
	
	if (dist2 <= radiusSum * radiusSum)
	{
		m_isHit = true;
	}
	else
	{
		m_isHit = false;
	}		
}
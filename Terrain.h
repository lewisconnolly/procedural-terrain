#pragma once

using namespace DirectX;

class Terrain
{
private:
	struct VertexType
	{
		DirectX::SimpleMath::Vector3 position;
		DirectX::SimpleMath::Vector2 texture;
		DirectX::SimpleMath::Vector3 normal;
	};
	struct HeightMapType
	{
		float x, y, z;
		float nx, ny, nz;
		float u, v;
	};

public:
	Terrain();
	~Terrain();

	bool Initialize(ID3D11Device*, int terrainWidth, int terrainHeight, float scalingFactor);
	void Render(ID3D11DeviceContext*);
	bool GenerateHeightMap(ID3D11Device*);	
	bool SmoothHeightMap(ID3D11Device*, int iterations);
	bool Update();
	float NewRandomHeight(int min, int max);

	float* GetWavelength();
	float* GetAmplitude();
	float* GetRoughness();
	float* GetRandomHeightLimit();
	DirectX::SimpleMath::Vector3 GetHeightMapPoint(DirectX::SimpleMath::Vector3 position);
	DirectX::SimpleMath::Vector3 Terrain::GetHeightMapPoint(int index);
	bool* GetRandomHeightEnabledState();
	int GetHeight();
	int GetWidth();

	bool MidpointDisplacement(ID3D11Device*, float dHeight, int sideLength, int tilesPerRow, int numRows, int numSquares = 1, int currentDepth = 0);

	SimpleMath::Vector3 AddCollisionDelta(SimpleMath::Vector3 camPos, float radius, bool sprinting, float delta);
	SimpleMath::Vector3 GetBoundedPosition(SimpleMath::Vector3 camPos, float radius, bool sprinting, float delta);

	float m_scalingFactor;

	float lerp(float begin, float end, float t)
	{
		return begin + t * (end - begin);
	}

private:
	bool CalculateNormals();
	void Shutdown();
	//void ShutdownBuffers();
	bool InitializeBuffers(ID3D11Device*);
	void RenderBuffers(ID3D11DeviceContext*);
	
	// Midpoint displacement functions
	void MpdInitialise(float dHeight, int width);
	int MpdGetSquareOriginIndex(int square, int depth, int sideLength);
	void MpdDiamond(int origin, float dHeight, int sideLength);
	void MpdSquare(int origin, float dHeight, int sideLength);
	void MpdSquareWrap(int origin, float dHeight, int sideLength);
	void MpdTileMap(int tilesPerRow, int numRows);	
	
private:
	bool m_terrainGeneratedToggle;
	bool m_randomHeightEnabled;
	int m_terrainWidth, m_terrainHeight, m_mpdTileWidth;
	ID3D11Buffer * m_vertexBuffer, *m_indexBuffer;
	int m_vertexCount, m_indexCount;
	float m_frequency, m_amplitude, m_wavelength, m_randomHeightLimit, m_roughness;
	HeightMapType* m_heightMap;
	HeightMapType* m_mpdMap;

	//arrays for our generated objects Made by directX
	std::vector<VertexPositionNormalTexture> preFabVertices;
	std::vector<uint16_t> preFabIndices;
};


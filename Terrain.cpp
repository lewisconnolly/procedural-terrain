#include "pch.h"
#include "Terrain.h"
#include <iostream>

Terrain::Terrain()
{
	m_terrainGeneratedToggle = false;
	m_randomHeightEnabled = false;
	srand(time(NULL));
}

Terrain::~Terrain()
{
}

bool Terrain::Initialize(ID3D11Device* device, int terrainWidth, int terrainHeight, float scalingFactor)
{
	int index;
	float height = 0.0;
	bool result;

	// Save the dimensions of the terrain.
	m_terrainWidth = terrainWidth;
	m_terrainHeight = terrainHeight;

	m_frequency = m_terrainWidth / 20;
	m_amplitude = 3.0;
	m_wavelength = 1.0;
	m_roughness = 1.4;
	m_randomHeightLimit = 3.0;

	m_scalingFactor = scalingFactor;

	// Create the structure to hold the terrain data.
	m_heightMap = new HeightMapType[m_terrainWidth * m_terrainHeight];
	if (!m_heightMap)
	{
		return false;
	}

	//this is how we calculate the texture coordinates first calculate the step size there will be between vertices. 
	//float textureCoordinatesStep = 5.0f / m_terrainWidth;  //tile 5 times across the terrain. 
	float textureCoordinatesStepW = 5.0f / m_terrainWidth;  //tile 5 times across the terrain. 
	float textureCoordinatesStepH = 5.0f / m_terrainHeight;  //tile 5 times across the terrain. 
	// Initialise the data in the height map (flat).
	for (int j = 0; j<m_terrainHeight; j++)
	{
		for (int i = 0; i<m_terrainWidth; i++)
		{
			index = (m_terrainWidth * j) + i;

			m_heightMap[index].x = (float)i;
			m_heightMap[index].y = (float)height;
			m_heightMap[index].z = (float)j;

			//and use this step to calculate the texture coordinates for this point on the terrain.
			/*m_heightMap[index].u = (float)i * textureCoordinatesStep;
			m_heightMap[index].v = (float)j * textureCoordinatesStep;*/
			m_heightMap[index].u = (float)i * textureCoordinatesStepW;
			m_heightMap[index].v = (float)j * textureCoordinatesStepH;

		}
	}

	//even though we are generating a flat terrain, we still need to normalise it. 
	// Calculate the normals for the terrain data.
	result = CalculateNormals();
	if (!result)
	{
		return false;
	}

	// Initialize the vertex and index buffer that hold the geometry for the terrain.
	result = InitializeBuffers(device);
	if (!result)
	{
		return false;
	}

	
	return true;
}

void Terrain::Render(ID3D11DeviceContext * deviceContext)
{
	// Put the vertex and index buffers on the graphics pipeline to prepare them for drawing.
	RenderBuffers(deviceContext);
	deviceContext->DrawIndexed(m_indexCount, 0, 0);

	return;
}

bool Terrain::CalculateNormals()
{
	int i, j, index1, index2, index3, index, count;
	float vertex1[3], vertex2[3], vertex3[3], vector1[3], vector2[3], sum[3], length;
	DirectX::SimpleMath::Vector3* normals;
	

	// Create a temporary array to hold the un-normalized normal vectors.
	normals = new DirectX::SimpleMath::Vector3[(m_terrainHeight - 1) * (m_terrainWidth - 1)];
	if (!normals)
	{
		return false;
	}

	// Go through all the faces in the mesh and calculate their normals.
	for (j = 0; j<(m_terrainHeight - 1); j++)
	{
		for (i = 0; i<(m_terrainWidth - 1); i++)
		{
			index1 = (m_terrainWidth * j) + i;
			index2 = (j * m_terrainWidth) + (i + 1);
			index3 = ((j + 1) * m_terrainWidth) + i;

			// Get three vertices from the face.
			vertex1[0] = m_heightMap[index1].x;
			vertex1[1] = m_heightMap[index1].y;
			vertex1[2] = m_heightMap[index1].z;

			vertex2[0] = m_heightMap[index2].x;
			vertex2[1] = m_heightMap[index2].y;
			vertex2[2] = m_heightMap[index2].z;

			vertex3[0] = m_heightMap[index3].x;
			vertex3[1] = m_heightMap[index3].y;
			vertex3[2] = m_heightMap[index3].z;

			// Calculate the two vectors for this face.
			vector1[0] = vertex1[0] - vertex3[0];
			vector1[1] = vertex1[1] - vertex3[1];
			vector1[2] = vertex1[2] - vertex3[2];
			vector2[0] = vertex3[0] - vertex2[0];
			vector2[1] = vertex3[1] - vertex2[1];
			vector2[2] = vertex3[2] - vertex2[2];

			index = (j * (m_terrainWidth - 1)) + i;

			// Calculate the cross product of those two vectors to get the un-normalized value for this face normal.
			normals[index].x = (vector1[1] * vector2[2]) - (vector1[2] * vector2[1]);
			normals[index].y = (vector1[2] * vector2[0]) - (vector1[0] * vector2[2]);
			normals[index].z = (vector1[0] * vector2[1]) - (vector1[1] * vector2[0]);
		}
	}

	// Now go through all the vertices and take an average of each face normal 	
	// that the vertex touches to get the averaged normal for that vertex.
	for (j = 0; j<m_terrainHeight; j++)
	{
		for (i = 0; i<m_terrainWidth; i++)
		{
			// Initialize the sum.
			sum[0] = 0.0f;
			sum[1] = 0.0f;
			sum[2] = 0.0f;

			// Initialize the count.
			count = 0;

			// Bottom left face.
			if (((i - 1) >= 0) && ((j - 1) >= 0))
			{
				index = ((j - 1) * (m_terrainWidth - 1)) + (i - 1);

				sum[0] += normals[index].x;
				sum[1] += normals[index].y;
				sum[2] += normals[index].z;
				count++;
			}

			// Bottom right face.
			if ((i < (m_terrainWidth - 1)) && ((j - 1) >= 0))
			{
				index = ((j - 1) * (m_terrainWidth - 1)) + i;

				sum[0] += normals[index].x;
				sum[1] += normals[index].y;
				sum[2] += normals[index].z;
				count++;
			}

			// Upper left face.
			if (((i - 1) >= 0) && (j < (m_terrainHeight - 1)))
			{
				index = (j * (m_terrainWidth - 1)) + (i - 1);

				sum[0] += normals[index].x;
				sum[1] += normals[index].y;
				sum[2] += normals[index].z;
				count++;
			}

			// Upper right face.
			if ((i < (m_terrainWidth - 1)) && (j < (m_terrainHeight - 1)))
			{
				index = (j * (m_terrainWidth - 1)) + i;

				sum[0] += normals[index].x;
				sum[1] += normals[index].y;
				sum[2] += normals[index].z;
				count++;
			}

			// Take the average of the faces touching this vertex.
			sum[0] = (sum[0] / (float)count);
			sum[1] = (sum[1] / (float)count);
			sum[2] = (sum[2] / (float)count);

			// Calculate the length of this normal.
			length = sqrt((sum[0] * sum[0]) + (sum[1] * sum[1]) + (sum[2] * sum[2]));

			// Get an index to the vertex location in the height map array.
			index = (j * m_terrainWidth) + i;

			// Normalize the final shared normal for this vertex and store it in the height map array.
			m_heightMap[index].nx = (sum[0] / length);
			m_heightMap[index].ny = (sum[1] / length);
			m_heightMap[index].nz = (sum[2] / length);
		}
	}

	// Release the temporary normals.
	delete[] normals;
	normals = 0;

	return true;
}

void Terrain::Shutdown()
{
	// Release the index buffer.
	if (m_indexBuffer)
	{
		m_indexBuffer->Release();
		m_indexBuffer = 0;
	}

	// Release the vertex buffer.
	if (m_vertexBuffer)
	{
		m_vertexBuffer->Release();
		m_vertexBuffer = 0;
	}

	return;
}

bool Terrain::InitializeBuffers(ID3D11Device * device )
{
	VertexType* vertices;
	unsigned long* indices;
	D3D11_BUFFER_DESC vertexBufferDesc, indexBufferDesc;
	D3D11_SUBRESOURCE_DATA vertexData, indexData;
	HRESULT result;
	int index, i, j;
	int index1, index2, index3, index4; //geometric indices. 

	// Calculate the number of vertices in the terrain mesh.
	m_vertexCount = (m_terrainWidth - 1) * (m_terrainHeight - 1) * 6;

	// Set the index count to the same as the vertex count.
	m_indexCount = m_vertexCount;

	// Create the vertex array.
	vertices = new VertexType[m_vertexCount];
	if (!vertices)
	{
		return false;
	}

	// Create the index array.
	indices = new unsigned long[m_indexCount];
	if (!indices)
	{
		return false;
	}

	// Initialize the index to the vertex buffer.
	index = 0;

	for (j = 0; j<(m_terrainHeight - 1); j++)
	{
		for (i = 0; i<(m_terrainWidth - 1); i++)
		{
			index1 = (m_terrainWidth * j) + i;          // Bottom left.
			index2 = (m_terrainWidth * j) + (i + 1);      // Bottom right.
			index3 = (m_terrainWidth * (j + 1)) + i;      // Upper left.
			index4 = (m_terrainWidth * (j + 1)) + (i + 1);  // Upper right.

			// Upper left.
			vertices[index].position = DirectX::SimpleMath::Vector3(m_heightMap[index3].x, m_heightMap[index3].y, m_heightMap[index3].z);
			vertices[index].normal = DirectX::SimpleMath::Vector3(m_heightMap[index3].nx, m_heightMap[index3].ny, m_heightMap[index3].nz);
			vertices[index].texture = DirectX::SimpleMath::Vector2(m_heightMap[index3].u, m_heightMap[index3].v);
			indices[index] = index;
			index++;

			// Upper right.
			vertices[index].position = DirectX::SimpleMath::Vector3(m_heightMap[index4].x, m_heightMap[index4].y, m_heightMap[index4].z);
			vertices[index].normal = DirectX::SimpleMath::Vector3(m_heightMap[index4].nx, m_heightMap[index4].ny, m_heightMap[index4].nz);
			vertices[index].texture = DirectX::SimpleMath::Vector2(m_heightMap[index4].u, m_heightMap[index4].v);
			indices[index] = index;
			index++;

			// Bottom left.
			vertices[index].position = DirectX::SimpleMath::Vector3(m_heightMap[index1].x, m_heightMap[index1].y, m_heightMap[index1].z);
			vertices[index].normal = DirectX::SimpleMath::Vector3(m_heightMap[index1].nx, m_heightMap[index1].ny, m_heightMap[index1].nz);
			vertices[index].texture = DirectX::SimpleMath::Vector2(m_heightMap[index1].u, m_heightMap[index1].v);
			indices[index] = index;
			index++;

			// Bottom left.
			vertices[index].position = DirectX::SimpleMath::Vector3(m_heightMap[index1].x, m_heightMap[index1].y, m_heightMap[index1].z);
			vertices[index].normal = DirectX::SimpleMath::Vector3(m_heightMap[index1].nx, m_heightMap[index1].ny, m_heightMap[index1].nz);
			vertices[index].texture = DirectX::SimpleMath::Vector2(m_heightMap[index1].u, m_heightMap[index1].v);
			indices[index] = index;
			index++;

			// Upper right.
			vertices[index].position = DirectX::SimpleMath::Vector3(m_heightMap[index4].x, m_heightMap[index4].y, m_heightMap[index4].z);
			vertices[index].normal = DirectX::SimpleMath::Vector3(m_heightMap[index4].nx, m_heightMap[index4].ny, m_heightMap[index4].nz);
			vertices[index].texture = DirectX::SimpleMath::Vector2(m_heightMap[index4].u, m_heightMap[index4].v);
			indices[index] = index;
			index++;

			// Bottom right.
			vertices[index].position = DirectX::SimpleMath::Vector3(m_heightMap[index2].x, m_heightMap[index2].y, m_heightMap[index2].z);
			vertices[index].normal = DirectX::SimpleMath::Vector3(m_heightMap[index2].nx, m_heightMap[index2].ny, m_heightMap[index2].nz);
			vertices[index].texture = DirectX::SimpleMath::Vector2(m_heightMap[index2].u, m_heightMap[index2].v);
			indices[index] = index;
			index++;
		}
	}

	// Set up the description of the static vertex buffer.
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth = sizeof(VertexType) * m_vertexCount;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	// Give the subresource structure a pointer to the vertex data.
	vertexData.pSysMem = vertices;
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;

	// Now create the vertex buffer.
	result = device->CreateBuffer(&vertexBufferDesc, &vertexData, &m_vertexBuffer);
	if (FAILED(result))
	{
		return false;
	}

	// Set up the description of the static index buffer.
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof(unsigned long) * m_indexCount;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;
	indexBufferDesc.StructureByteStride = 0;

	// Give the subresource structure a pointer to the index data.
	indexData.pSysMem = indices;
	indexData.SysMemPitch = 0;
	indexData.SysMemSlicePitch = 0;

	// Create the index buffer.
	result = device->CreateBuffer(&indexBufferDesc, &indexData, &m_indexBuffer);
	if (FAILED(result))
	{
		return false;
	}

	// Release the arrays now that the vertex and index buffers have been created and loaded.
	delete[] vertices;
	vertices = 0;

	delete[] indices;
	indices = 0;

	return true;
}

void Terrain::RenderBuffers(ID3D11DeviceContext * deviceContext)
{
	unsigned int stride;
	unsigned int offset;

	// Set vertex buffer stride and offset.
	stride = sizeof(VertexType);
	offset = 0;

	// Set the vertex buffer to active in the input assembler so it can be rendered.
	deviceContext->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);

	// Set the index buffer to active in the input assembler so it can be rendered.
	deviceContext->IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);

	// Set the type of primitive that should be rendered from this vertex buffer, in this case triangles.
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	return;
}

bool Terrain::GenerateHeightMap(ID3D11Device* device)
{
	bool result;

	int index;
	float height = 0;
	float heightX = 0;
	float heightZ = 0;

	m_frequency = (6.283/m_terrainHeight) / m_wavelength; //we want a wavelength of 1 to be a single wave over the whole terrain.  A single wave is 2 pi which is about 6.283	

	//loop through the terrain and set the hieghts how we want. This is where we generate the terrain
	//in this case I will run a sin-wave through the terrain in x axis.
	//cos-wave through terrain in z-axis
	for (int j = 0; j<m_terrainHeight; j++)
	{
		for (int i = 0; i<m_terrainWidth; i++)
		{
			if (m_randomHeightEnabled)
			{
				heightX = NewRandomHeight((int)-m_randomHeightLimit, (int)m_randomHeightLimit);
				heightZ = 0;
			}
			else
			{
				heightX = (float)(sin((float)i * m_frequency) * m_amplitude);
				heightZ = (float)(cos((float)j * m_frequency) * m_amplitude);;
			}
			
			index = (m_terrainWidth * j) + i;

			m_heightMap[index].x = (float)i;			
			m_heightMap[index].y = heightX + heightZ;
			m_heightMap[index].z = (float)j;
		}
	}

	result = CalculateNormals();
	if (!result)
	{
		return false;
	}

	result = InitializeBuffers(device);
	if (!result)
	{
		return false;
	}
}

float Terrain::NewRandomHeight(int min, int max)
{	
	int MIN = min;
	int MAX = max;

	if (MIN > MAX)
	{
		MAX = min;
		MIN = max;
	}
	else if (MIN == MAX)
	{
		MIN = min - 1;
		MAX = max;
	}
	
	int ri = (rand() % (MAX - MIN)) + MIN; // Random integer between min and max
	float rm = (float)(rand()) / (float)(RAND_MAX); // Random mantissa part of float number
	float rf = (float)ri + rm; // Combine integer part and mantissa for random float
	return rf;
}

bool Terrain::SmoothHeightMap(ID3D11Device* device, int iterations)
{
	bool result;
	int index;
	float height = 0;
	
	for (int n = 0; n < iterations; n++)
	{
		for (int j = 0; j < m_terrainHeight; j++)
		{
			for (int i = 0; i < m_terrainWidth; i++)
			{
				index = (m_terrainWidth * j) + i;

				if (j == 0) // bottom row
				{

					if (i == m_terrainWidth - 1) // bottom right corner
					{
						height =
							// point above
							(((m_heightMap[index + m_terrainHeight].y +
								// point at up left diagonal
								m_heightMap[index + m_terrainHeight - 1].y +
								// point to left
								m_heightMap[index - 1].y) / 3.0) +
								// current point;
								m_heightMap[index].y) / 2.0;
					}
					else if (i == 0) // bottom left corner
					{
						height =
							// point above
							(((m_heightMap[index + m_terrainHeight].y +
								// point at up right diagonal
								m_heightMap[index + m_terrainHeight + 1].y +
								// point to right
								m_heightMap[index + 1].y) / 3.0) +
								// current point
								m_heightMap[index].y) / 2.0;
					}
					else
					{
						height =
							// point above
							(((m_heightMap[index + m_terrainHeight].y +
								// point at up right diagonal
								m_heightMap[index + m_terrainHeight + 1].y +
								// point at up left diagonal
								m_heightMap[index + m_terrainHeight - 1].y +
								// point to left
								m_heightMap[index - 1].y +
								// point to right
								m_heightMap[index + 1].y) / 5.0) +
								// current point
								m_heightMap[index].y) / 2.0;
					}
				}
				else if (j == m_terrainHeight - 1) // top row
				{
					if (i == m_terrainWidth - 1) // top right corner
					{
						height =
							// point below 
							(((m_heightMap[index - m_terrainHeight].y +
								// point at down left diagonal
								m_heightMap[index - m_terrainHeight - 1].y +
								// point to left
								m_heightMap[index - 1].y) / 3.0) +
								// current point
								m_heightMap[index].y) / 2.0;
					}
					else if (i == 0) // top left corner
					{
						height =
							// point below 
							(((m_heightMap[index - m_terrainHeight].y +
								// point at down right diagonal
								m_heightMap[index - m_terrainHeight + 1].y +
								// point to right
								m_heightMap[index + 1].y) / 3.0) +
								// current point
								m_heightMap[index].y) / 2.0;
					}
					else
					{
						height =
							// point below 
							(((m_heightMap[index - m_terrainHeight].y +
								// point at down right diagonal
								m_heightMap[index - m_terrainHeight + 1].y +
								// point at down left diagonal
								m_heightMap[index - m_terrainHeight - 1].y +
								// point to left
								m_heightMap[index - 1].y +
								// point to right
								m_heightMap[index + 1].y) / 5.0) +
								// current point
								m_heightMap[index].y) / 2.0;
					}
				}
				else if (i == 0) // left side
				{
					height =
						// point below 
						(((m_heightMap[index - m_terrainHeight].y +
							// point above
							m_heightMap[index + m_terrainHeight].y +
							// point at down right diagonal
							m_heightMap[index - m_terrainHeight + 1].y +
							// point at up right diagonal
							m_heightMap[index + m_terrainHeight + 1].y +
							// point to right
							m_heightMap[index + 1].y) / 5.0) +
							// current point
							m_heightMap[index].y) / 2.0;
				}
				else if (i == m_terrainWidth - 1) // right side
				{
					height =
						// point below 
						(((m_heightMap[index - m_terrainHeight].y +
							// point above
							m_heightMap[index + m_terrainHeight].y +
							// point at up left diagonal
							m_heightMap[index + m_terrainHeight - 1].y +
							// point at down left diagonal
							m_heightMap[index - m_terrainHeight - 1].y +
							// point to left
							m_heightMap[index - 1].y) / 5.0) +
							// current point
							m_heightMap[index].y) / 2.0;
				}
				else // any other point
				{
					height =
						// point below 
						(((m_heightMap[index - m_terrainHeight].y +
							// point above
							m_heightMap[index + m_terrainHeight].y +
							// point at down right diagonal
							m_heightMap[index - m_terrainHeight + 1].y +
							// point at up right diagonal
							m_heightMap[index + m_terrainHeight + 1].y +
							// point at up left diagonal
							m_heightMap[index + m_terrainHeight - 1].y +
							// point at down left diagonal
							m_heightMap[index - m_terrainHeight - 1].y +
							// point to left
							m_heightMap[index - 1].y +
							// point to right
							m_heightMap[index + 1].y) / 8.0) +
							// current point
							m_heightMap[index].y) / 2.0;
				}

				m_heightMap[index].x = (float)i;
				m_heightMap[index].y = height;
				m_heightMap[index].z = (float)j;
			}
		}
	}

	result = CalculateNormals();
	if (!result)
	{
		return false;
	}

	result = InitializeBuffers(device);
	if (!result)
	{
		return false;
	}
}

bool Terrain::MidpointDisplacement(ID3D11Device* device, float dHeight, int sideLength, int tilesPerRow, int numRows, int numSquares, int currentDepth)
{	
	int curSquareOriginIndex;

	if (currentDepth == 0)
	{
		// Seed corners of terrain
		MpdInitialise(dHeight, sideLength);
	}	

	// Diamond step
	for (int square = 0; square < numSquares; square++)
	{
		curSquareOriginIndex = MpdGetSquareOriginIndex(square, currentDepth, sideLength);

		// Set height of diamond point
		MpdDiamond(curSquareOriginIndex, dHeight, sideLength);
	}

	// Square step
	for (int square = 0; square < numSquares; square++)
	{
		curSquareOriginIndex = MpdGetSquareOriginIndex(square, currentDepth, sideLength);

		// Set height of square points
		MpdSquareWrap(curSquareOriginIndex, dHeight, sideLength);
	}		

	int nextSideLength = (sideLength + 1) / 2;

	// Stop when inner squares are too small to have midpoints (sideLength = 2)
	if (nextSideLength == 2)
	{
		bool result;

		MpdTileMap(tilesPerRow, numRows);
		
		result = CalculateNormals();
		if (!result)
		{
			return false;
		}

		result = InitializeBuffers(device);
		if (!result)
		{
			return false;
		}

		return true;
	}
	else // Run again, dividing terrain into smaller squares
	{						
		float nextLeveldHeight = dHeight * pow(2, -m_roughness);
		int nextLevelNumSquares = numSquares * 4;		
		int nextDepth = currentDepth + 1;
		
		MidpointDisplacement(device, nextLeveldHeight, nextSideLength, tilesPerRow, numRows, nextLevelNumSquares, nextDepth);
	}	
}

void Terrain::MpdTileMap(int tilesPerRow, int numRows)
{	
	int smallWidth = m_mpdTileWidth;
	int smallArea = smallWidth * smallWidth;

	int largeWidth = m_terrainWidth;
	int largeHeight = m_terrainHeight;
	int largeArea = m_terrainWidth * m_terrainHeight;

	int largeIndex;
	int innerTileRowJump;

	for (int r = 0; r < numRows; r++)
	{
		for (int t = 0; t < tilesPerRow; t++)
		{
			innerTileRowJump = 0;

			for (int i = 0; i < smallArea; i++)
			{
				if (i != 0 && i % smallWidth == 0) { innerTileRowJump += largeWidth - smallWidth; }

				largeIndex = i + (smallWidth * t) + (largeWidth * smallWidth * r) + innerTileRowJump;

				m_heightMap[largeIndex].y = m_mpdMap[i].y;
			}
		}
	}
}

void Terrain::MpdInitialise(float dHeight, int width)
{
	int index = 0;
	float height = 0.0;
	m_mpdTileWidth = width;
	m_mpdMap = new HeightMapType[m_mpdTileWidth * m_mpdTileWidth];
	float textureCoordinatesStep = 5.0f / m_mpdTileWidth;  //tile 5 times across the terrain. 			
	int limit = round(dHeight/2.0);
	float cornerHeight = NewRandomHeight(-limit, limit);

	for (int j = 0; j < m_mpdTileWidth; j++)
	{
		for (int i = 0; i < m_mpdTileWidth; i++)
		{
			index = (m_mpdTileWidth * j) + i;

			m_mpdMap[index].x = (float)i;
			m_mpdMap[index].y = height;
			m_mpdMap[index].z = (float)j;

			// Top left corner
			if (i == 0 && j == (m_mpdTileWidth - 1))
			{
				m_mpdMap[index].y = cornerHeight;
			}

			// Top right corner
			if (i == (m_mpdTileWidth - 1) && j == (m_mpdTileWidth - 1))
			{
				m_mpdMap[index].y = cornerHeight;
			}

			// Bottom left corner
			if (i == 0 && j == 0)
			{
				m_mpdMap[index].y = cornerHeight;
			}

			// Bottom right corner
			if (i == (m_mpdTileWidth - 1) && j == 0)
			{
				m_mpdMap[index].y = cornerHeight;
			}

			//use this step to calculate the texture coordinates for this point on the terrain.
			m_mpdMap[index].u = (float)i * textureCoordinatesStep;
			m_mpdMap[index].v = (float)j * textureCoordinatesStep;
		}
	}
}

void Terrain::MpdDiamond(int origin, float dHeight, int sideLength)
{
	int index;
	float ll, lr, ul, ur;
	ll = 0;
	lr = 0;
	ul = 0;
	ur = 0;
	int distFromULandLR = (m_mpdTileWidth - 1) * (sideLength - 1) / 2;
	int distFromLLandUR = (m_mpdTileWidth + 1) * (sideLength - 1) / 2;
	int centerIndex = origin + (m_mpdTileWidth + 1) * (sideLength - 1) / 2;
	int limit = round(dHeight / 2.0);

	ll = m_mpdMap[centerIndex - distFromLLandUR].y; // lower left corner
	lr = m_mpdMap[centerIndex - distFromULandLR].y; // lower right
	ul = m_mpdMap[centerIndex + distFromULandLR].y; // upper left
	ur = m_mpdMap[centerIndex + distFromLLandUR].y; // upper right

	// Make center point average of corners plus random displacement
	m_mpdMap[centerIndex].y = (ll + lr + ul + ur) / 4 + NewRandomHeight(-limit, limit);
}

void Terrain::MpdSquare(int origin, float dHeight, int sideLength)
{
	//int wrapAroundIndex = 0;
	float u, d, l, r;
	u = 0;
	d = 0;
	l = 0;
	r = 0;
	int limit = round(dHeight / 2);
	
	/* Top midpoint */
	int topMid = origin + 513 * (sideLength - 1) + (sideLength - 1) / 2; 
	
	d = m_mpdMap[topMid - 513 * (sideLength - 1) / 2].y;
	l = m_mpdMap[topMid - (sideLength - 1) / 2].y;
	r = m_mpdMap[topMid + (sideLength - 1) / 2].y;

	// Check if at edge
	if (topMid + 513 > 513 * 513)
	{
		m_mpdMap[topMid].y = (d + l + r) / 3 + NewRandomHeight(-limit, limit);
	}
	else
	{
		u = m_mpdMap[topMid + 513 * (sideLength - 1) / 2].y;
		
		m_mpdMap[topMid].y = (u + d + l + r) / 4 + NewRandomHeight(-limit, limit);
	}

	/* Bottom midpoint */
	int bottomMid = origin + (sideLength - 1) / 2;
	
	u = m_mpdMap[bottomMid + 513 * (sideLength - 1) / 2].y;
	l = m_mpdMap[bottomMid - (sideLength - 1) / 2].y;
	r = m_mpdMap[bottomMid + (sideLength - 1) / 2].y;

	// Check if at edge
	if (bottomMid - 513 < 0)
	{
		m_mpdMap[bottomMid].y = (u + l + r) / 3 + NewRandomHeight(-limit, limit);
	}
	else
	{
		d = m_mpdMap[bottomMid - 513 * (sideLength - 1) / 2].y;
		
		m_mpdMap[bottomMid].y = (u + l + r + d) / 4 + NewRandomHeight(-limit, limit);
	}

	/* Left midpoint */
	int leftMid = origin + 513 * (sideLength - 1) / 2;
	
	u = m_mpdMap[leftMid + 513 * (sideLength - 1) / 2].y;
	d = m_mpdMap[leftMid - 513 * (sideLength - 1) / 2].y;
	r = m_mpdMap[leftMid + (sideLength - 1) / 2].y;

	// Check if at edge
	if (leftMid % 513 == 0)
	{
		m_mpdMap[leftMid].y = (u + d + r) / 3 + NewRandomHeight(-limit, limit);
	}
	else
	{
		l = m_mpdMap[leftMid - (sideLength - 1) / 2].y;
		
		m_mpdMap[leftMid].y = (u + d + l + r) / 4 + NewRandomHeight(-limit, limit);
	}

	/* Right midpoint */
	int rightMid = origin + 513 * (sideLength - 1) / 2 + sideLength - 1;

	u = m_mpdMap[rightMid + 513 * (sideLength - 1) / 2].y;
	d = m_mpdMap[rightMid - 513 * (sideLength - 1) / 2].y;
	l = m_mpdMap[rightMid - (sideLength - 1) / 2].y;

	// Check if at edge
	if ((rightMid + 1) % 513 == 0)
	{
		m_mpdMap[rightMid].y = (u + d + l) / 3 + NewRandomHeight(-limit, limit);
	}
	else
	{
		r = m_mpdMap[rightMid + (sideLength - 1) / 2].y;
		
		m_mpdMap[rightMid].y = (u + d + l + r) / 4 + NewRandomHeight(-limit, limit);
	}
}

void Terrain::MpdSquareWrap(int origin, float dHeight, int sideLength)
{
	float u, d, l, r;
	u = 0;
	d = 0;
	l = 0;
	r = 0;
	int limit = round(dHeight / 2);
	int wrapAroundIndex = 0;

	/* Top midpoint */
	int topMid = origin + m_mpdTileWidth * (sideLength - 1) + (sideLength - 1) / 2;

	d = m_mpdMap[topMid - m_mpdTileWidth * (sideLength - 1) / 2].y;
	l = m_mpdMap[topMid - (sideLength - 1) / 2].y;
	r = m_mpdMap[topMid + (sideLength - 1) / 2].y;

	// Check if at edge
	if (topMid + m_mpdTileWidth > m_mpdTileWidth * m_mpdTileWidth)
	{
		// Wrap around for point "above"
		wrapAroundIndex = topMid - (m_mpdTileWidth - (sideLength + 1) / 2) * m_mpdTileWidth;
		u = m_mpdMap[wrapAroundIndex].y;

		m_mpdMap[topMid].y = (d + l + r + u) / 4;

	}
	else
	{
		u = m_mpdMap[topMid + m_mpdTileWidth * (sideLength - 1) / 2].y;

		m_mpdMap[topMid].y = (u + d + l + r) / 4 + NewRandomHeight(-limit, limit);
	}

	/* Bottom midpoint */
	int bottomMid = origin + (sideLength - 1) / 2;

	u = m_mpdMap[bottomMid + m_mpdTileWidth * (sideLength - 1) / 2].y;
	l = m_mpdMap[bottomMid - (sideLength - 1) / 2].y;
	r = m_mpdMap[bottomMid + (sideLength - 1) / 2].y;

	// Check if at edge
	if (bottomMid - m_mpdTileWidth < 0)
	{
		// Wrap around for point "below"
		wrapAroundIndex = bottomMid + (m_mpdTileWidth - (sideLength + 1) / 2) * m_mpdTileWidth;
		d = m_mpdMap[wrapAroundIndex].y;

		m_mpdMap[bottomMid].y = (u + l + r + d) / 4;

	}
	else
	{
		d = m_mpdMap[bottomMid - m_mpdTileWidth * (sideLength - 1) / 2].y;

		m_mpdMap[bottomMid].y = (u + l + r + d) / 4 + NewRandomHeight(-limit, limit);
	}

	/* Left midpoint */
	int leftMid = origin + m_mpdTileWidth * (sideLength - 1) / 2;

	u = m_mpdMap[leftMid + m_mpdTileWidth * (sideLength - 1) / 2].y;
	d = m_mpdMap[leftMid - m_mpdTileWidth * (sideLength - 1) / 2].y;
	r = m_mpdMap[leftMid + (sideLength - 1) / 2].y;

	// Check if at edge
	if (leftMid % m_mpdTileWidth == 0)
	{
		// Wrap around for point to "left"
		wrapAroundIndex = leftMid + m_mpdTileWidth - (sideLength + 1) / 2;
		l = m_mpdMap[wrapAroundIndex].y;

		m_mpdMap[leftMid].y = (u + d + r + l) / 4;
	}
	else
	{
		l = m_mpdMap[leftMid - (sideLength - 1) / 2].y;

		m_mpdMap[leftMid].y = (u + d + l + r) / 4 + NewRandomHeight(-limit, limit);
	}

	/* Right midpoint */
	int rightMid = origin + m_mpdTileWidth * (sideLength - 1) / 2 + sideLength - 1;

	u = m_mpdMap[rightMid + m_mpdTileWidth * (sideLength - 1) / 2].y;
	d = m_mpdMap[rightMid - m_mpdTileWidth * (sideLength - 1) / 2].y;
	l = m_mpdMap[rightMid - (sideLength - 1) / 2].y;

	// Check if at edge
	if ((rightMid + 1) % m_mpdTileWidth == 0)
	{
		// Wrap around for point to "right"
		wrapAroundIndex = rightMid - m_mpdTileWidth + (sideLength + 1) / 2;
		r = m_mpdMap[wrapAroundIndex].y;

		m_mpdMap[rightMid].y = (u + d + l + r) / 4;
	}
	else
	{
		r = m_mpdMap[rightMid + (sideLength - 1) / 2].y;

		m_mpdMap[rightMid].y = (u + d + l + r) / 4 + NewRandomHeight(-limit, limit);
	}
}

int Terrain::MpdGetSquareOriginIndex(int square, int depth, int sideLength)
{
	int index = 0;
	int x = 0;
	int z = 0;

	if (square == 0)
	{
		return index;
	}
	else
	{
		int numSquaresPerRow = pow(2, depth);		
		// creates sequence that increases by sideLength / numSquaresPerRow for numSquaresPerRow numbers then repeats
		x = (square % numSquaresPerRow) * (sideLength - 1);
		// creates sequence of same number numSquaresPerRow times then increases by sideLength / numSquaresPerRow then repeats
		z = floor(square / numSquaresPerRow) * (sideLength - 1);

		index = x + z * m_mpdTileWidth;
	}

	return index;
}

bool Terrain::Update()
{
	return true; 
}

float* Terrain::GetWavelength()
{
	return &m_wavelength;
}

float* Terrain::GetAmplitude()
{
	return &m_amplitude;
}

float* Terrain::GetRoughness()
{
	return &m_roughness;
}

float* Terrain::GetRandomHeightLimit()
{
	return &m_randomHeightLimit;
}

bool* Terrain::GetRandomHeightEnabledState()
{
	return &m_randomHeightEnabled;
}

DirectX::SimpleMath::Vector3 Terrain::GetHeightMapPoint(DirectX::SimpleMath::Vector3 position)
{
	int heightMapIndex = (float)m_terrainWidth * round(position.z) + round(position.x);	
	
	if (heightMapIndex > 0 && heightMapIndex < m_terrainWidth * m_terrainHeight)
	{
		return DirectX::SimpleMath::Vector3(m_heightMap[heightMapIndex].x, m_heightMap[heightMapIndex].y, m_heightMap[heightMapIndex].z);
	}
	else
	{
		DirectX::SimpleMath::Vector3().Zero;
	}
}

DirectX::SimpleMath::Vector3 Terrain::GetHeightMapPoint(int index)
{	
	if (index >= 0 && index < m_terrainWidth * m_terrainHeight)
	{
		return DirectX::SimpleMath::Vector3(m_heightMap[index].x, m_heightMap[index].y, m_heightMap[index].z);
	}
	else
	{
		DirectX::SimpleMath::Vector3().Zero;
	}
}

int Terrain::GetWidth()
{
	return m_terrainWidth;
}

int Terrain::GetHeight()
{
	return m_terrainHeight;
}

SimpleMath::Vector3 Terrain::AddCollisionDelta(SimpleMath::Vector3 camPos, float radius, bool sprinting, float delta)
{		
	//SimpleMath::Vector3 camPos = m_Camera01.getPosition();
	SimpleMath::Vector3 m_terrainPoint = GetHeightMapPoint(camPos / m_scalingFactor);
	DirectX::SimpleMath::Vector3 newPostition = camPos;

	if (m_terrainPoint != DirectX::SimpleMath::Vector3().Zero)
	{
		bool collisionResult = true;

		/*if (round(camPos.y - radius) <= round(m_terrainPoint.y * m_scalingFactor))
		{
			collisionResult = true;
		}
		else
		{
			collisionResult = false;
		}*/

		if (collisionResult)
		{
			float shiftDelta = m_terrainPoint.y * m_scalingFactor - camPos.y + radius;

			if (sprinting)
			{
				newPostition = DirectX::SimpleMath::Vector3(camPos.x, camPos.y + shiftDelta, camPos.z);
			}
			else
			{
				// Interpolate to above ground position if not sprinting to prevent noticeable jumps in cam position
				float lerpAmount = 1.0 - pow(0.00001, delta);
				float lerpY = lerp(camPos.y, camPos.y + shiftDelta, lerpAmount);
				newPostition = DirectX::SimpleMath::Vector3(camPos.x, lerpY, camPos.z);
			}

			return newPostition;
		}

		return newPostition;
	}		
}

SimpleMath::Vector3 Terrain::GetBoundedPosition(SimpleMath::Vector3 camPos, float radius, bool sprinting, float delta)
{

	SimpleMath::Vector3 m_terrainPoint = GetHeightMapPoint(camPos / m_scalingFactor);
	DirectX::SimpleMath::Vector3 newPostition = camPos;
	float lerpAmount = 1.0 - pow(0.00001, delta);
	float lerpY = camPos.y;

	if (m_terrainPoint != DirectX::SimpleMath::Vector3().Zero)
	{
		float shiftDelta = m_terrainPoint.y * m_scalingFactor - camPos.y + radius;

		if (sprinting)
		{
			newPostition = DirectX::SimpleMath::Vector3(camPos.x, camPos.y + shiftDelta, camPos.z);
		}
		else
		{
			// Interpolate to above ground position if not sprinting to prevent noticeable jumps in cam position			
			float lerpY = lerp(camPos.y, camPos.y + shiftDelta, lerpAmount);
			newPostition = DirectX::SimpleMath::Vector3(camPos.x, lerpY, camPos.z);
		}

		if (camPos.x / m_scalingFactor > m_terrainWidth) newPostition.x = m_terrainWidth * m_scalingFactor;
		if (camPos.x / m_scalingFactor < 0) newPostition.x = 0;
		if ((camPos.z + 100) / m_scalingFactor > m_terrainHeight) newPostition.z = (m_terrainWidth - 100) * m_scalingFactor;
		if ((camPos.z - 100) / m_scalingFactor < 0) newPostition.z = 100 * m_scalingFactor;

		float lerpZ = lerp(camPos.z, newPostition.z, lerpAmount);
		
		newPostition = DirectX::SimpleMath::Vector3(newPostition.x, newPostition.y, lerpZ);

		return newPostition;		
	}
}
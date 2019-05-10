#include <d3d11.h>
#include <d3dcompiler.h>
#include <iostream>

struct ExampleInputSR
{
	float a;
	int b;
};
struct ExampleInputCB
{
	float c;
	int d;
	int padding1;
	int padding2;
};
struct ExampleOutput {
	float r;
};

ExampleInputSR inputArray[] = {
	{5.56f, 10},
	{2.51f, 74},
	{4.52f, 50},
	{5.56f, 15},
	{6.76f, 22},
	{8.34f, 17},
	{1.77f, 48},
	{2.53f, 14},
	{0.23f, 74},
	{3.12f, 94}
};
ExampleInputCB inputCB = { 1.00f, 2 };
ExampleOutput outputArray[ARRAYSIZE(inputArray)];

int main() 
{
	ID3D11Device* device = nullptr;
	ID3D11DeviceContext* deviceContext = nullptr;

	ID3D11ComputeShader* exampleShader = nullptr;

	ID3D11Buffer* inputArrayBuffer = nullptr;
	ID3D11Buffer* inputCBBuffer = nullptr;
	ID3D11Buffer* outputArrayBufferGPU = nullptr;
	ID3D11Buffer* outputArrayBufferCPU = nullptr;

	ID3D11ShaderResourceView* inputArraySRV = nullptr;
	ID3D11UnorderedAccessView* outputArrayUAV = nullptr;

	//SETUP PHASE:
	//==========================================================================================================
		//Create Device:
		{
		D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_DEBUG, nullptr, 0, D3D11_SDK_VERSION, &device, nullptr, &deviceContext);
	}

		//Create Shader:
		{
		ID3D10Blob* shaderData = nullptr;
		
		D3DReadFileToBlob((LPCWSTR)L"ComputeShader.cso", &shaderData);
		device->CreateComputeShader(shaderData->GetBufferPointer(), shaderData->GetBufferSize(), nullptr, &exampleShader);

		shaderData->Release();
	}

		//Setup Input:
		{
			//SRV:
			{
				//1. Create buffer:
				D3D11_BUFFER_DESC bd = {};
				bd.ByteWidth = sizeof(ExampleInputSR)*ARRAYSIZE(inputArray);
				bd.Usage = D3D11_USAGE_DEFAULT;
				bd.BindFlags = D3D11_BIND_SHADER_RESOURCE;
				bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
				bd.StructureByteStride = sizeof(ExampleInputSR);
				bd.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

				D3D11_SUBRESOURCE_DATA srd = {};
				srd.pSysMem = inputArray;
				srd.SysMemPitch = 0;
				srd.SysMemSlicePitch = 0;

				device->CreateBuffer(&bd, &srd, &inputArrayBuffer);

				//2. Create view into that buffer:
				D3D11_SHADER_RESOURCE_VIEW_DESC srvd = {};
				srvd.Format = DXGI_FORMAT_UNKNOWN;
				srvd.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
				srvd.Buffer.ElementOffset = 0;
				srvd.Buffer.ElementWidth = sizeof(ExampleInputSR);
				srvd.Buffer.FirstElement = 0;
				srvd.Buffer.NumElements = ARRAYSIZE(inputArray);

				device->CreateShaderResourceView(inputArrayBuffer, &srvd, &inputArraySRV);
			}

			//CB:
			{
				D3D11_BUFFER_DESC bd = {};
				bd.ByteWidth = sizeof(ExampleInputCB);
				bd.Usage = D3D11_USAGE_DEFAULT;
				bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
				bd.CPUAccessFlags = 0u;
				bd.StructureByteStride = 0u;
				bd.MiscFlags = 0u;

				D3D11_SUBRESOURCE_DATA srd = {};
				srd.pSysMem = &inputCB;
				srd.SysMemPitch = 0;
				srd.SysMemSlicePitch = 0;

				device->CreateBuffer(&bd, &srd, &inputCBBuffer);
			}
		}

		//Setup Output:
		{
			//for Main UAV:
			D3D11_BUFFER_DESC bd = {};
			bd.ByteWidth = sizeof(ExampleOutput)*ARRAYSIZE(outputArray);
			bd.Usage = D3D11_USAGE_DEFAULT;
			bd.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
			bd.CPUAccessFlags = 0;
			bd.StructureByteStride = sizeof(ExampleOutput);
			bd.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

			device->CreateBuffer(&bd, 0, &outputArrayBufferGPU);

			D3D11_UNORDERED_ACCESS_VIEW_DESC uavd = {};
			uavd.Buffer.FirstElement = 0;
			uavd.Buffer.Flags = 0;
			uavd.Buffer.NumElements = ARRAYSIZE(outputArray);
			uavd.Format = DXGI_FORMAT_UNKNOWN;
			uavd.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
			UINT uavInitialCount = 0;

			device->CreateUnorderedAccessView(outputArrayBufferGPU, &uavd, &outputArrayUAV);

			//for copying back:

			D3D11_SUBRESOURCE_DATA srd = {};
			srd.pSysMem = outputArray;
			srd.SysMemPitch = 0;
			srd.SysMemSlicePitch = 0;

			bd.Usage = D3D11_USAGE_STAGING;
			bd.BindFlags = 0;
			bd.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

			device->CreateBuffer(&bd, &srd, &outputArrayBufferCPU);
		}
	//==========================================================================================================


	//EXECUTION PHASE:
	//==========================================================================================================
		//Setup Input, Output and Shader:
		{
			UINT uavInitialCount = 0;
			deviceContext->CSSetShader(exampleShader, nullptr, 0);
			deviceContext->CSSetShaderResources(0, 1, &inputArraySRV);
			deviceContext->CSSetConstantBuffers(0, 1, &inputCBBuffer);
			deviceContext->CSSetUnorderedAccessViews(0, 1, &outputArrayUAV, &uavInitialCount);

			deviceContext->UpdateSubresource(inputArrayBuffer, 0, 0, inputArray, 0, 0);
			deviceContext->UpdateSubresource(inputCBBuffer, 0, 0, &inputCB, 0, 0);
		}

		//Execute:
		{
			deviceContext->Dispatch(ARRAYSIZE(inputArray), 1, 1);
		}

		//Read Back:
		{
			deviceContext->CopyResource(outputArrayBufferCPU, outputArrayBufferGPU);
			D3D11_MAPPED_SUBRESOURCE mappedResource;
			deviceContext->Map(outputArrayBufferCPU, 0, D3D11_MAP_READ, 0, &mappedResource);
			memcpy(outputArray, mappedResource.pData, sizeof(ExampleOutput)*ARRAYSIZE(outputArray));

			//For debugging:
			bool correct = true;
			for (int i = 0; i < ARRAYSIZE(outputArray); i++) {
				float result = float(inputArray[i].a * inputCB.c) + float(inputArray[i].b * inputCB.d);
				std::cout << outputArray[i].r << "\n";
				if (result != outputArray[i].r) {
					correct = false;
					break;
				}
			}

			if (correct) {
				std::cout << "All results are correct.";
			}
		}

	//==========================================================================================================

	//CleanUp:
	device->Release();
	deviceContext->Release();
	exampleShader->Release();
	inputArrayBuffer->Release();
	inputCBBuffer->Release();
	outputArrayBufferGPU->Release();
	outputArrayBufferCPU->Release();
	inputArraySRV->Release();
	outputArrayUAV->Release();

	std::cin.get();
}

//TODO: to implement resizing of input and output buffers.

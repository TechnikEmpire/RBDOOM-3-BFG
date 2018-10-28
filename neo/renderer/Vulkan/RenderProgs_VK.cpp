/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2018 Robert Beckebans
Copyright (C) 2016-2017 Dustin Land

This file is part of the Doom 3 BFG Edition GPL Source Code ("Doom 3 BFG Edition Source Code").

Doom 3 BFG Edition Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 BFG Edition Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 BFG Edition Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 BFG Edition Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 BFG Edition Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#pragma hdrstop
#include "precompiled.h"

#include "../RenderCommon.h"
#include "../RenderProgs.h"


void RpPrintState( uint64 stateBits, uint64* stencilBits );

struct vertexLayout_t
{
	VkPipelineVertexInputStateCreateInfo inputState;
	idList< VkVertexInputBindingDescription > bindingDesc;
	idList< VkVertexInputAttributeDescription > attributeDesc;
};

idUniformBuffer emptyUBO;

static vertexLayout_t vertexLayouts[ NUM_VERTEX_LAYOUTS ];

static const char* renderProgBindingStrings[ BINDING_TYPE_MAX ] =
{
	"ubo",
	"sampler"
};

/*
=============
CreateVertexDescriptions
=============
*/
void CreateVertexDescriptions()
{
	VkPipelineVertexInputStateCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	createInfo.pNext = NULL;
	createInfo.flags = 0;
	
	VkVertexInputBindingDescription binding = {};
	VkVertexInputAttributeDescription attribute = {};
	
	{
		vertexLayout_t& layout = vertexLayouts[ LAYOUT_DRAW_VERT ];
		layout.inputState = createInfo;
		
		uint32 locationNo = 0;
		uint32 offset = 0;
		
		binding.stride = sizeof( idDrawVert );
		binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		layout.bindingDesc.Append( binding );
		
		// Position
		attribute.format = VK_FORMAT_R32G32B32_SFLOAT;
		attribute.location = locationNo++;
		attribute.offset = offset;
		layout.attributeDesc.Append( attribute );
		offset += sizeof( idDrawVert::xyz );
		
		// TexCoord
		attribute.format = VK_FORMAT_R16G16_SFLOAT;
		attribute.location = locationNo++;
		attribute.offset = offset;
		layout.attributeDesc.Append( attribute );
		offset += sizeof( idDrawVert::st );
		
		// Normal
		attribute.format = VK_FORMAT_R8G8B8A8_UNORM;
		attribute.location = locationNo++;
		attribute.offset = offset;
		layout.attributeDesc.Append( attribute );
		offset += sizeof( idDrawVert::normal );
		
		// Tangent
		attribute.format = VK_FORMAT_R8G8B8A8_UNORM;
		attribute.location = locationNo++;
		attribute.offset = offset;
		layout.attributeDesc.Append( attribute );
		offset += sizeof( idDrawVert::tangent );
		
		// Color1
		attribute.format = VK_FORMAT_R8G8B8A8_UNORM;
		attribute.location = locationNo++;
		attribute.offset = offset;
		layout.attributeDesc.Append( attribute );
		offset += sizeof( idDrawVert::color );
		
		// Color2
		attribute.format = VK_FORMAT_R8G8B8A8_UNORM;
		attribute.location = locationNo++;
		attribute.offset = offset;
		layout.attributeDesc.Append( attribute );
	}
	
	{
		vertexLayout_t& layout = vertexLayouts[ LAYOUT_DRAW_SHADOW_VERT_SKINNED ];
		layout.inputState = createInfo;
		
		uint32 locationNo = 0;
		uint32 offset = 0;
		
		binding.stride = sizeof( idShadowVertSkinned );
		binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		layout.bindingDesc.Append( binding );
		
		// Position
		attribute.format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attribute.location = locationNo++;
		attribute.offset = offset;
		layout.attributeDesc.Append( attribute );
		offset += sizeof( idShadowVertSkinned::xyzw );
		
		// Color1
		attribute.format = VK_FORMAT_R8G8B8A8_UNORM;
		attribute.location = locationNo++;
		attribute.offset = offset;
		layout.attributeDesc.Append( attribute );
		offset += sizeof( idShadowVertSkinned::color );
		
		// Color2
		attribute.format = VK_FORMAT_R8G8B8A8_UNORM;
		attribute.location = locationNo++;
		attribute.offset = offset;
		layout.attributeDesc.Append( attribute );
	}
	
	{
		vertexLayout_t& layout = vertexLayouts[ LAYOUT_DRAW_SHADOW_VERT ];
		layout.inputState = createInfo;
		
		binding.stride = sizeof( idShadowVert );
		binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		layout.bindingDesc.Append( binding );
		
		// Position
		attribute.format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attribute.location = 0;
		attribute.offset = 0;
		layout.attributeDesc.Append( attribute );
	}
}

/*
========================
CreateDescriptorPools
========================
*/
void CreateDescriptorPools( VkDescriptorPool( &pools )[ NUM_FRAME_DATA ] )
{
	const int numPools = 2;
	VkDescriptorPoolSize poolSizes[ numPools ];
	poolSizes[ 0 ].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[ 0 ].descriptorCount = MAX_DESC_UNIFORM_BUFFERS;
	poolSizes[ 1 ].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[ 1 ].descriptorCount = MAX_DESC_IMAGE_SAMPLERS;
	
	VkDescriptorPoolCreateInfo poolCreateInfo = {};
	poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolCreateInfo.pNext = NULL;
	poolCreateInfo.maxSets = MAX_DESC_SETS;
	poolCreateInfo.poolSizeCount = numPools;
	poolCreateInfo.pPoolSizes = poolSizes;
	
	for( int i = 0; i < NUM_FRAME_DATA; ++i )
	{
		ID_VK_CHECK( vkCreateDescriptorPool( vkcontext.device, &poolCreateInfo, NULL, &pools[ i ] ) );
	}
}


/*
========================
idRenderProgManager::StartFrame
========================
*/
void idRenderProgManager::StartFrame()
{

}

/*
================================================================================================
idRenderProgManager::BindProgram
================================================================================================
*/
void idRenderProgManager::BindProgram( int index )
{
	if( current == index )
	{
		return;
	}
	
	current = index;
	
	RENDERLOG_PRINTF( "Binding SPIR-V Program %s\n", renderProgs[ index ].name.c_str() );
}

/*
================================================================================================
idRenderProgManager::Unbind
================================================================================================
*/
void idRenderProgManager::Unbind()
{
	current = -1;
}

/*
================================================================================================
idRenderProgManager::LoadShader
================================================================================================
*/
void idRenderProgManager::LoadShader( int index, rpStage_t stage )
{
	if( shaders[ index ].module != VK_NULL_HANDLE )
	{
		return; // Already loaded
	}
	
	LoadShader( shaders[index] );
}

/*
================================================================================================
idRenderProgManager::LoadGLSLShader
================================================================================================
*/
void idRenderProgManager::LoadShader( shader_t& shader )
{
	idStr inFile;
	idStr outFileHLSL;
	idStr outFileGLSL;
	idStr outFileSPIRV;
	idStr outFileLayout;
	
	// RB: replaced backslashes
	inFile.Format( "renderprogs/%s", shader.name.c_str() );
	inFile.StripFileExtension();
	outFileHLSL.Format( "renderprogs/hlsl/%s%s", shader.name.c_str(), shader.nameOutSuffix.c_str() );
	outFileHLSL.StripFileExtension();
	
	outFileGLSL.Format( "renderprogs/vkglsl/%s%s", shader.name.c_str(), shader.nameOutSuffix.c_str() );
	outFileLayout.Format( "renderprogs/vkglsl/%s%s", shader.name.c_str(), shader.nameOutSuffix.c_str() );
	
	outFileGLSL.StripFileExtension();
	outFileLayout.StripFileExtension();
	
	if( shader.stage == SHADER_STAGE_FRAGMENT )
	{
		inFile += ".ps.hlsl";
		outFileHLSL += ".ps.hlsl";
		outFileGLSL += ".frag";
		outFileLayout += ".frag.layout";
		outFileSPIRV += ".fspv";
	}
	else
	{
		inFile += ".vs.hlsl";
		outFileHLSL += ".vs.hlsl";
		outFileGLSL += ".vert";
		outFileLayout += ".vert.layout";
		outFileSPIRV += ".vspv";
	}
	
	// first check whether we already have a valid GLSL file and compare it to the hlsl timestamp;
	ID_TIME_T hlslTimeStamp;
	int hlslFileLength = fileSystem->ReadFile( inFile.c_str(), NULL, &hlslTimeStamp );
	
	ID_TIME_T glslTimeStamp;
	int glslFileLength = fileSystem->ReadFile( outFileGLSL.c_str(), NULL, &glslTimeStamp );
	
	// if the glsl file doesn't exist or we have a newer HLSL file we need to recreate the glsl file.
	idStr programGLSL;
	idStr programLayout;
	//if( ( glslFileLength <= 0 ) || ( hlslTimeStamp != FILE_NOT_FOUND_TIMESTAMP && hlslTimeStamp > glslTimeStamp ) || r_alwaysExportGLSL.GetBool() )
	{
		const char* hlslFileBuffer = NULL;
		int len = 0;
		
		if( hlslFileLength <= 0 )
		{
			hlslFileBuffer = FindEmbeddedSourceShader( inFile.c_str() );
			if( hlslFileBuffer == NULL )
			{
				// hlsl file doesn't even exist bail out
				idLib::Error( "HLSL file %s could not be loaded and may be corrupt", inFile.c_str() );
				return;
			}
			
			len = strlen( hlslFileBuffer );
		}
		else
		{
			len = fileSystem->ReadFile( inFile.c_str(), ( void** ) &hlslFileBuffer );
		}
		
		if( len <= 0 )
		{
			return;
		}
		
		idStrList compileMacros;
		for( int j = 0; j < MAX_SHADER_MACRO_NAMES; j++ )
		{
			if( BIT( j ) & shader.shaderFeatures )
			{
				const char* macroName = GetGLSLMacroName( ( shaderFeature_t ) j );
				compileMacros.Append( idStr( macroName ) );
			}
		}
		
		idStr hlslCode( hlslFileBuffer );
		idStr programHLSL = StripDeadCode( hlslCode, inFile, compileMacros, shader.builtin );
		programGLSL = ConvertCG2GLSL( programHLSL, inFile, shader.stage == SHADER_STAGE_VERTEX, programLayout, true );
		
		fileSystem->WriteFile( outFileHLSL, programHLSL.c_str(), programHLSL.Length(), "fs_savepath" );
		fileSystem->WriteFile( outFileGLSL, programGLSL.c_str(), programGLSL.Length(), "fs_savepath" );
		fileSystem->WriteFile( outFileLayout, programLayout.c_str(), programLayout.Length(), "fs_savepath" );
	}
	/*
	else
	{
		// read in the glsl file
		void* fileBufferGLSL = NULL;
		int lengthGLSL = fileSystem->ReadFile( outFileGLSL.c_str(), &fileBufferGLSL );
		if( lengthGLSL <= 0 )
		{
			idLib::Error( "GLSL file %s could not be loaded and may be corrupt", outFileGLSL.c_str() );
		}
		programGLSL = ( const char* ) fileBufferGLSL;
		Mem_Free( fileBufferGLSL );
	
	
		{
			// read in the uniform file
			void* fileBufferUniforms = NULL;
			int lengthUniforms = fileSystem->ReadFile( outFileUniforms.c_str(), &fileBufferUniforms );
			if( lengthUniforms <= 0 )
			{
				idLib::Error( "uniform file %s could not be loaded and may be corrupt", outFileUniforms.c_str() );
			}
			programUniforms = ( const char* ) fileBufferUniforms;
			Mem_Free( fileBufferUniforms );
		}
	}
	*/
	
	// RB: find the uniforms locations in either the vertex or fragment uniform array
	// this uses the new layout structure
	{
		idLexer src( outFileLayout, outFileLayout.Length(), "layout" );
		idToken token;
		if( src.ExpectTokenString( "uniforms" ) )
		{
			src.ExpectTokenString( "[" );
			
			while( !src.CheckTokenString( "]" ) )
			{
				src.ReadToken( &token );
				
				int index = -1;
				for( int i = 0; i < RENDERPARM_TOTAL && index == -1; i++ )
				{
					const char* parmName = GetGLSLParmName( i );
					if( token == parmName )
					{
						index = i;
					}
				}
				
				if( index == -1 )
				{
					idLib::Error( "couldn't find uniform %s for %s", token.c_str(), outFileGLSL.c_str() );
				}
				shader.parmIndices.Append( index );
			}
		}
		
		if( src.ExpectTokenString( "bindings" ) )
		{
			src.ExpectTokenString( "[" );
			
			while( !src.CheckTokenString( "]" ) )
			{
				src.ReadToken( &token );
				
				int index = -1;
				for( int i = 0; i < BINDING_TYPE_MAX; ++i )
				{
					if( token == renderProgBindingStrings[ i ] )
					{
						index = i;
					}
				}
				
				if( index == -1 )
				{
					idLib::Error( "Invalid binding %s", token.c_str() );
				}
				
				shader.bindings.Append( static_cast< rpBinding_t >( index ) );
			}
		}
	}
	
	// TODO GLSL to SPIR-V compilation
	
#if 0
	
	// create and compile the shader
	shader.progId = glCreateShader( glTarget );
	if( shader.progId )
	{
		const char* source[1] = { programGLSL.c_str() };
		
		glShaderSource( shader.progId, 1, source, NULL );
		glCompileShader( shader.progId );
		
		int infologLength = 0;
		glGetShaderiv( shader.progId, GL_INFO_LOG_LENGTH, &infologLength );
		if( infologLength > 1 )
		{
			idTempArray<char> infoLog( infologLength );
			int charsWritten = 0;
			glGetShaderInfoLog( shader.progId, infologLength, &charsWritten, infoLog.Ptr() );
			
			// catch the strings the ATI and Intel drivers output on success
			if( strstr( infoLog.Ptr(), "successfully compiled to run on hardware" ) != NULL ||
					strstr( infoLog.Ptr(), "No errors." ) != NULL )
			{
				//idLib::Printf( "%s program %s from %s compiled to run on hardware\n", typeName, GetName(), GetFileName() );
			}
			else if( r_displayGLSLCompilerMessages.GetBool() ) // DG:  check for the CVar I added above
			{
				idLib::Printf( "While compiling %s program %s\n", ( shader.stage == SHADER_STAGE_FRAGMENT ) ? "fragment" : "vertex" , inFile.c_str() );
				
				const char separator = '\n';
				idList<idStr> lines;
				lines.Clear();
				idStr source( programGLSL );
				lines.Append( source );
				for( int index = 0, ofs = lines[index].Find( separator ); ofs != -1; index++, ofs = lines[index].Find( separator ) )
				{
					lines.Append( lines[index].c_str() + ofs + 1 );
					lines[index].CapLength( ofs );
				}
				
				idLib::Printf( "-----------------\n" );
				for( int i = 0; i < lines.Num(); i++ )
				{
					idLib::Printf( "%3d: %s\n", i + 1, lines[i].c_str() );
				}
				idLib::Printf( "-----------------\n" );
				
				idLib::Printf( "%s\n", infoLog.Ptr() );
			}
		}
		
		GLint compiled = GL_FALSE;
		glGetShaderiv( shader.progId, GL_COMPILE_STATUS, &compiled );
		if( compiled == GL_FALSE )
		{
			glDeleteShader( shader.progId );
			return;
		}
	}
#endif
}

/*
================================================================================================
idRenderProgManager::LoadGLSLProgram
================================================================================================
*/
void idRenderProgManager::LoadGLSLProgram( const int programIndex, const int vertexShaderIndex, const int fragmentShaderIndex )
{
	renderProg_t& prog = renderProgs[programIndex];
	
	if( prog.progId != INVALID_PROGID )
	{
		return; // Already loaded
	}
	
	idStr programName = shaders[ vertexShaderIndex ].name;
	programName.StripFileExtension();
	prog.name = programName;
	//prog.progId = programIndex;
	prog.fragmentShaderIndex = fragmentShaderIndex;
	prog.vertexShaderIndex = vertexShaderIndex;
	
	// TODO
	//CreateDescriptorSetLayout( shaders[ vertexShaderIndex ], shaders[ fragmentShaderIndex ], prog );
	
#if 0
	// RB: removed idStr::Icmp( name, "heatHaze.vfp" ) == 0  hack
	for( int i = 0; i < shaders[vertexShaderIndex].uniforms.Num(); i++ )
	{
		if( shaders[vertexShaderIndex].uniforms[i] == RENDERPARM_ENABLE_SKINNING )
		{
			prog.usesJoints = true;
			prog.optionalSkinning = true;
		}
	}
#endif
}

/*
================================================================================================
idRenderProgManager::CommitUnforms
================================================================================================
*/
void idRenderProgManager::CommitUniforms( uint64 stateBits )
{
#if 0
	const int progID = current;
	const renderProg_t& prog = renderProgs[progID];
	
	//GL_CheckErrors();
	
	ALIGNTYPE16 idVec4 localVectors[RENDERPARM_TOTAL];
	
	auto commitarray = [&]( idVec4( &vectors )[ RENDERPARM_TOTAL ] , shader_t& shader )
	{
		const int numUniforms = shader.uniforms.Num();
		if( shader.uniformArray != -1 && numUniforms > 0 )
		{
			int totalUniforms = 0;
			for( int i = 0; i < numUniforms; ++i )
			{
				// RB: HACK rpShadowMatrices[6 * 4]
				if( shader.uniforms[i] == RENDERPARM_SHADOW_MATRIX_0_X )
				{
					for( int j = 0; j < ( 6 * 4 ); j++ )
					{
						vectors[i + j] = uniforms[ shader.uniforms[i] + j];
						totalUniforms++;
					}
					
				}
				else
				{
					vectors[i] = uniforms[ shader.uniforms[i] ];
					totalUniforms++;
				}
			}
			glUniform4fv( shader.uniformArray, totalUniforms, localVectors->ToFloatPtr() );
		}
	};
	
	if( prog.vertexShaderIndex >= 0 )
	{
		commitarray( localVectors, shaders[ prog.vertexShaderIndex ] );
	}
	
	if( prog.fragmentShaderIndex >= 0 )
	{
		commitarray( localVectors, shaders[ prog.fragmentShaderIndex ] );
	}
	
#endif
}





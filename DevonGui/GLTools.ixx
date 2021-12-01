module;

#include <stdio.h>
#include <vector>
#include "gl43.h"
#include "LogWindow.h"

export module GLTools;

namespace GLTools
{
	namespace ShaderId
	{
		export enum Value
		{
			Primary,
			CRT,
			BloomH,
			BloomV,
			Final,
			Max
		};
	}

	namespace RenderTextureId
	{
		export enum Value
		{
			Primary,
			CRT,
			BloomH,
			BloomV,
			Max
		};
	}

	static GLuint   g_GlVersion = 0;              // Extracted at runtime using GL_MAJOR_VERSION, GL_MINOR_VERSION queries (e.g. 320 for GL 3.2)
	static char     g_GlslVersionString[] = "#version 130\n";   // Specified by user or detected based on compile time GL settings.

	static GLuint	g_FramebufferName[RenderTextureId::Max];
	static GLuint	g_RenderTexture[RenderTextureId::Max];
	static GLuint	g_DepthRenderBuffer = 0;

	export std::vector<uint32_t> PrimaryBuffer;

	struct PPVertex
	{
		struct{ float X; float Y; } Pos;
		struct{ float U; float V; } UV;
	};

	static const PPVertex QuadVerts[] = { 
		{ {-1.f, -1.f}, {0.f, 0.f}  },
		{ {+1.f, +1.f}, {1.f, 1.f}  },
		{ {+1.f, -1.f}, {1.f, 0.f}  },
		{ {-1.f, -1.f}, {0.f, 0.f}  },
		{ {-1.f, +1.f}, {0.f, 1.f}  },
		{ {+1.f, +1.f}, {1.f, 1.f}  },
	};

	export char PPVS[] =
        "in vec2 Position;\n"
        "in vec2 UV;\n"
        "out vec2 Frag_UV;\n"
        "void main()\n"
        "{\n"
        "    Frag_UV = UV;\n"
        "    gl_Position = vec4(Position.xy,0,1);\n"
        "}\n";

	export char FinalPS[] =
        "uniform sampler2D Texture0;\n"
        "uniform sampler2D Texture1;\n"
        "uniform float BloomAmount;\n"
		"in vec2 Frag_UV;\n"
		"out vec4 Out_Color;\n"
        "void main()\n"
        "{\n"
		"	vec4 t0 = texture(Texture0, Frag_UV);\n"
		"	vec4 t1 = texture(Texture1, Frag_UV);\n"
		"	Out_Color = mix(t0 + t1 * BloomAmount, mix(t0, t1, BloomAmount), .5);\n"
        "}\n";

	export char BloomHPS[] =
        "uniform sampler2D Texture;\n"
        "uniform float Scanline;\n"
        "uniform float BloomRadius;\n"
		"in vec2 Frag_UV;\n"
		"out vec4 Out_Color;\n"
        "#define BLUR_SMP 12.\n"
        "#define BLUR_CURVE_POW 2.2\n"
        "void main()\n"
        "{\n"
		"	vec4 col = vec4(0);\n"
		"	vec2 unit = vec2(1. / Scanline);\n"
		"	float count = 0.0;\n"
		"	for(float x = -BloomRadius; x < BloomRadius; x += (BloomRadius/BLUR_SMP))\n"
		"	{\n"
		"		float weight = pow(BloomRadius - abs(x), BLUR_CURVE_POW);\n"
		"		vec4 Source = texture(Texture, Frag_UV + vec2(x * unit.x, Frag_UV.y * unit.y) );\n"
		"		float L = mix(.333 * (Source.r + Source.g + Source.b), max(Source.r, max(Source.g, Source.b)), .5);\n"
		"		col += Source * weight * (1.5*pow(L, 1.2) + .5);\n"
		"		count += weight;\n"
		"	}\n"
		"	Out_Color = col / count;\n"
        "}\n";

	export char BloomVPS[] =
        "uniform sampler2D Texture;\n"
        "uniform float Scanline;\n"
        "uniform float BloomRadius;\n"
		"in vec2 Frag_UV;\n"
		"out vec4 Out_Color;\n"
        "#define BLUR_SMP 12.\n"
        "#define BLUR_CURVE_POW 2.2\n"
        "void main()\n"
        "{\n"
		"	vec4 col = vec4(0);\n"
		"	vec2 unit = vec2(1. / Scanline);\n"
		"	float count = 0.0;\n"
		"	for(float x = -BloomRadius; x < BloomRadius; x += (BloomRadius/BLUR_SMP))\n"
		"	{\n"
		"		float weight = pow(BloomRadius - abs(x), BLUR_CURVE_POW);\n"
		"		vec4 Source = texture(Texture, Frag_UV + vec2(Frag_UV.x * unit.x, x * unit.y) );\n"
		"		float L = mix(.333 * (Source.r + Source.g + Source.b), max(Source.r, max(Source.g, Source.b)), .5);\n"
		"		col += Source * weight * (1.5*pow(L, 1.2) + .5);\n"
		"		count += weight;\n"
		"	}\n"
		"	Out_Color = col / count;\n"
        "}\n";

	GLuint glQuadBuffer;

	export const uint32_t PrimaryH = 512;
	const uint32_t PrimaryW = 512;

	int PreviousW = 0;
	int PreviousH = 0;

	struct ShaderHandles
	{
		GLuint	g_ShaderHandle = 0, g_VertHandle = 0, g_FragHandle = 0;
		GLint   g_AttribLocationProjMtx = 0;
		GLint   g_AttribLocationViewMtx = 0;
		GLint   g_AttribLocationTransMtx = 0;
		GLint   g_AttribLocationScanline = 0;
		GLuint  g_AttribLocationPos = 0, g_AttribLocationNormal = 0, g_AttribLocationColor = 0; // Vertex attributes location
		GLuint  g_AttribLocationTex0 = 0;
		GLuint  g_AttribLocationTex1 = 0;
		GLuint  g_AttribLocationUV = 0;
	};

	GLint   g_AttribLocationNbScanlines = 0;
	GLint   g_AttribLocationScanline = 0;
	GLint   g_AttribLocationRoundness = 0;
	GLint   g_AttribLocationBorderSharpness = 0;
	GLint   g_AttribLocationVignetting = 0;
	GLint   g_AttribLocationBrightness = 0;
	GLint   g_AttribLocationContrast = 0;
	GLint   g_AttribLocationSharpness = 0;
	GLint   g_AttribLocationGridDep = 0;
	GLint   g_AttribLocationGhostAmount = 0;
	GLint   g_AttribLocationChromaAmount = 0;
	GLint   g_AttribLocationBloomAmount = 0;
	GLint   g_AttribLocationBloomRadius = 0;

	static ShaderHandles ShaderTab[ShaderId::Max];

	
	export void * FileDiagCreateTex(uint8_t* data, int w, int h, char fmt)
	{
		union { GLuint tex[2]; void * V; } Ret;

		glGenTextures(1, &Ret.tex[0]);
		glBindTexture(GL_TEXTURE_2D, Ret.tex[0]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, (fmt == 0) ? GL_BGRA : GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);

		return Ret.V;
	};

	export void SetRenderTarget(RenderTextureId::Value TextureId)
	{
		if(TextureId == RenderTextureId::Max)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
		else
		{
			glBindFramebuffer(GL_FRAMEBUFFER, g_FramebufferName[TextureId]);
			if(TextureId == RenderTextureId::Primary)
			{
				glClearColor(.2f, .2f, .2f, 1.f);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			}
		}
	}

	export void UpdateRenderTargets(int width, int height)
	{
		if(PreviousW != width || PreviousH != height)
		{
			for(int i = 1; i < RenderTextureId::Max; i++)
			{
				glBindTexture(GL_TEXTURE_2D, g_RenderTexture[i]);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

				glBindFramebuffer(GL_FRAMEBUFFER, g_FramebufferName[i]);
				glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, g_RenderTexture[i], 0);
				GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
				glDrawBuffers(1, DrawBuffers);
			}
		}

		PreviousW = width;
		PreviousH = height;
	}

	export bool ConstructRenderTargets(int width, int height)
	{
		PrimaryBuffer.resize(512*512);

		glGenFramebuffers(RenderTextureId::Max, g_FramebufferName);
		glGenTextures(RenderTextureId::Max, g_RenderTexture);

		glBindFramebuffer(GL_FRAMEBUFFER, g_FramebufferName[RenderTextureId::Primary]);
	
		glBindTexture(GL_TEXTURE_2D, g_RenderTexture[RenderTextureId::Primary]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, PrimaryW, PrimaryH, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

		glGenRenderbuffers(1, &g_DepthRenderBuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, g_DepthRenderBuffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, PrimaryW, PrimaryH);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, g_DepthRenderBuffer);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, g_RenderTexture[0], 0);
		GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
		glDrawBuffers(1, DrawBuffers);

		UpdateRenderTargets(width, height);

		if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			return false;

		return true;
	}

	export void UpdatePrimaryTexture()
	{
		glBindTexture(GL_TEXTURE_2D, g_RenderTexture[RenderTextureId::Primary]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, PrimaryW, PrimaryH, 0, GL_RGBA, GL_UNSIGNED_BYTE, PrimaryBuffer.data());
	}

	static bool CheckShader(GLuint handle, const char* desc, LogWindow & Log)
	{
		GLint status = 0, log_length = 0;
		glGetShaderiv(handle, GL_COMPILE_STATUS, &status);
		glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &log_length);
		if ((GLboolean)status == GL_FALSE)
			Log.AddLog("ERROR: CreateDeviceObjects: failed to compile %s!\n", desc);
		if (log_length > 1)
		{
			std::vector<char> buf;
			buf.resize(size_t(log_length) + 1);
			glGetShaderInfoLog(handle, log_length, NULL, (GLchar*)buf.data());
			Log.AddLog("%s\n", buf.data());
		}
		return (GLboolean)status == GL_TRUE;
	}

	static bool CheckProgram(GLuint handle, const char* desc, LogWindow & Log)
	{
		GLint status = 0, log_length = 0;
		glGetProgramiv(handle, GL_LINK_STATUS, &status);
		glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &log_length);
		if ((GLboolean)status == GL_FALSE)
			Log.AddLog("ERROR: CreateDeviceObjects: failed to link %s! (with GLSL '%s')\n", desc, g_GlslVersionString);
		if (log_length > 1)
		{
			std::vector<char> buf;
			buf.resize(size_t(log_length) + 1);
			glGetProgramInfoLog(handle, log_length, NULL, (GLchar*)buf.data());
			Log.AddLog("%s\n", buf.data());
		}
		return (GLboolean)status == GL_TRUE;
	}

	export void GlVersion()
	{
		GLint major = 0;
		GLint minor = 0;
		glGetIntegerv(GL_MAJOR_VERSION, &major);
		glGetIntegerv(GL_MINOR_VERSION, &minor);
		if (major == 0 && minor == 0)
		{
			// Query GL_VERSION in desktop GL 2.x, the string will start with "<major>.<minor>"
			const char* gl_version = (const char*)glGetString(GL_VERSION);
			sscanf_s(gl_version, "%d.%d", &major, &minor);
		}
		g_GlVersion = (GLuint)(major * 100 + minor * 10);
	}

	export void CompileShaders(ShaderId::Value Id, LogWindow & Log, const char * PShaderTxt, const char * VShaderTxt)
	{
		ShaderHandles & Shader = ShaderTab[Id];

		// Backup GL state
		GLint last_texture, last_array_buffer, last_vertex_array;
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
		glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
		glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array);

		// Create shaders
		const GLchar* vertex_shader_with_version[2] = { g_GlslVersionString, VShaderTxt };
		Shader.g_VertHandle = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(Shader.g_VertHandle, 2, vertex_shader_with_version, NULL);
		glCompileShader(Shader.g_VertHandle);
		CheckShader(Shader.g_VertHandle, "vertex shader", Log);

		const GLchar* fragment_shader_with_version[2] = { g_GlslVersionString, PShaderTxt };
		Shader.g_FragHandle = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(Shader.g_FragHandle, 2, fragment_shader_with_version, NULL);
		glCompileShader(Shader.g_FragHandle);
		CheckShader(Shader.g_FragHandle, "fragment shader", Log);

		Shader.g_ShaderHandle = glCreateProgram();
		glAttachShader(Shader.g_ShaderHandle, Shader.g_VertHandle);
		glAttachShader(Shader.g_ShaderHandle, Shader.g_FragHandle);
		glLinkProgram(Shader.g_ShaderHandle);
		CheckProgram(Shader.g_ShaderHandle, "shader program", Log);

		// Restore modified GL state
		glBindTexture(GL_TEXTURE_2D, last_texture);
		glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
		glBindVertexArray(last_vertex_array);

		switch(Id)
		{
		case ShaderId::Primary:
			Shader.g_AttribLocationProjMtx = glGetUniformLocation(Shader.g_ShaderHandle, "ProjMtx");
			Shader.g_AttribLocationTransMtx = glGetUniformLocation(Shader.g_ShaderHandle, "TransMtx");
			Shader.g_AttribLocationViewMtx = glGetUniformLocation(Shader.g_ShaderHandle, "ViewMtx");
			Shader.g_AttribLocationPos = glGetAttribLocation(Shader.g_ShaderHandle, "Position");
			Shader.g_AttribLocationNormal = glGetAttribLocation(Shader.g_ShaderHandle, "Normal");
			Shader.g_AttribLocationColor = glGetAttribLocation(Shader.g_ShaderHandle, "Color");
			break;

		case ShaderId::CRT:
			Shader.g_AttribLocationPos = glGetAttribLocation(Shader.g_ShaderHandle, "Position");
			Shader.g_AttribLocationUV = glGetAttribLocation(Shader.g_ShaderHandle, "UV");
		    Shader.g_AttribLocationTex0 = glGetUniformLocation(Shader.g_ShaderHandle, "Texture");
			g_AttribLocationNbScanlines = glGetUniformLocation(Shader.g_ShaderHandle, "NbScanlines");
			g_AttribLocationScanline = glGetUniformLocation(Shader.g_ShaderHandle, "Scanline");
			g_AttribLocationRoundness = glGetUniformLocation(Shader.g_ShaderHandle, "Roundness");
			g_AttribLocationBorderSharpness = glGetUniformLocation(Shader.g_ShaderHandle, "BorderSharpness");
			g_AttribLocationVignetting = glGetUniformLocation(Shader.g_ShaderHandle, "Vignetting");
			g_AttribLocationBrightness = glGetUniformLocation(Shader.g_ShaderHandle, "Brightness");
			g_AttribLocationContrast = glGetUniformLocation(Shader.g_ShaderHandle, "Contrast");
			g_AttribLocationSharpness = glGetUniformLocation(Shader.g_ShaderHandle, "Sharpness");
			g_AttribLocationGridDep = glGetUniformLocation(Shader.g_ShaderHandle, "GridDep");
			g_AttribLocationGhostAmount = glGetUniformLocation(Shader.g_ShaderHandle, "GhostAmount");
			g_AttribLocationChromaAmount = glGetUniformLocation(Shader.g_ShaderHandle, "ChromaAmount");						
			break;
		
		case ShaderId::BloomH:
		case ShaderId::BloomV:
			Shader.g_AttribLocationScanline = glGetUniformLocation(Shader.g_ShaderHandle, "Scanline");
			Shader.g_AttribLocationPos = glGetAttribLocation(Shader.g_ShaderHandle, "Position");
			Shader.g_AttribLocationUV = glGetAttribLocation(Shader.g_ShaderHandle, "UV");
			g_AttribLocationBloomRadius = glGetUniformLocation(Shader.g_ShaderHandle, "BloomRadius");
			break;

		case ShaderId::Final:
			Shader.g_AttribLocationPos = glGetAttribLocation(Shader.g_ShaderHandle, "Position");
			Shader.g_AttribLocationUV = glGetAttribLocation(Shader.g_ShaderHandle, "UV");
		    Shader.g_AttribLocationTex0 = glGetUniformLocation(Shader.g_ShaderHandle, "Texture0");
		    Shader.g_AttribLocationTex1 = glGetUniformLocation(Shader.g_ShaderHandle, "Texture1");
			g_AttribLocationBloomAmount = glGetUniformLocation(Shader.g_ShaderHandle, "BloomAmount");
			break;
		}
	}

	export void DestroyShaders(ShaderId::Value Id)
	{
		ShaderHandles & Shader = ShaderTab[Id];

		if(Shader.g_ShaderHandle && Shader.g_VertHandle) { glDetachShader(Shader.g_ShaderHandle, Shader.g_VertHandle); }
		if(Shader.g_ShaderHandle && Shader.g_FragHandle) { glDetachShader(Shader.g_ShaderHandle, Shader.g_FragHandle); }
		if(Shader.g_VertHandle)       { glDeleteShader(Shader.g_VertHandle); Shader.g_VertHandle = 0; }
		if(Shader.g_FragHandle)       { glDeleteShader(Shader.g_FragHandle); Shader.g_FragHandle = 0; }
		if(Shader.g_ShaderHandle)     { glDeleteProgram(Shader.g_ShaderHandle); Shader.g_ShaderHandle = 0; }
	}

	export void ReCompileShaders(ShaderId::Value Id, LogWindow & Log, const char * PShaderTxt, const char * VShaderTxt)
	{
		DestroyShaders(Id);
		CompileShaders(Id, Log, PShaderTxt, VShaderTxt);
	}

	export void SetupVertexAttribs(ShaderId::Value Id)
	{
		ShaderHandles & Shader = ShaderTab[Id];
		switch(Id)
		{
		case ShaderId::Primary:
		case ShaderId::BloomH:
		case ShaderId::BloomV:
		case ShaderId::CRT:
		case ShaderId::Final:
			glEnableVertexAttribArray(Shader.g_AttribLocationPos);
			glEnableVertexAttribArray(Shader.g_AttribLocationUV);
			glVertexAttribPointer(Shader.g_AttribLocationPos,	2, GL_FLOAT, GL_FALSE,	sizeof(PPVertex), (GLvoid*)IM_OFFSETOF(PPVertex, Pos));
			glVertexAttribPointer(Shader.g_AttribLocationUV,	2, GL_FLOAT, GL_FALSE,	sizeof(PPVertex), (GLvoid*)IM_OFFSETOF(PPVertex, UV));
			break;
		}
	}

	export void ConstructQuadBuffers()
	{
		glGenBuffers(1, &glQuadBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, glQuadBuffer);
		glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(PPVertex), QuadVerts, GL_STATIC_DRAW);
	}

	export void DestroyBuffers()
	{
		glDeleteBuffers(1, &glQuadBuffer);
		glDeleteFramebuffers(RenderTextureId::Max, g_FramebufferName);
		glDeleteTextures(RenderTextureId::Max, g_RenderTexture);
		glDeleteBuffers(1, &g_DepthRenderBuffer);
	}

	export void RenderFinal(RenderTextureId::Value SourceTexture0, RenderTextureId::Value SourceTexture1, int width, int height, float BloomAmount)
	{
		glViewport(0, 0, width, height);
		glUseProgram(ShaderTab[ShaderId::Final].g_ShaderHandle);
		glUniform1f(g_AttribLocationBloomAmount, BloomAmount);
		
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, g_RenderTexture[SourceTexture0]);
		glUniform1i(ShaderTab[ShaderId::Final].g_AttribLocationTex0, 0);
		
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, g_RenderTexture[SourceTexture1]);
		glUniform1i(ShaderTab[ShaderId::Final].g_AttribLocationTex1, 1);
		glActiveTexture(GL_TEXTURE0);

		glBindBuffer( GL_ARRAY_BUFFER, glQuadBuffer);
		SetupVertexAttribs(ShaderId::Final);
		glDrawArrays(GL_TRIANGLES, 0, 6);
	}

	export void RenderBloomH(RenderTextureId::Value SourceTexture, int width, int height, float Scanline, float BloomRadius)
	{
		glViewport(0, 0, width, height);
		glUseProgram(ShaderTab[ShaderId::BloomH].g_ShaderHandle);
		glUniform1f(ShaderTab[ShaderId::BloomH].g_AttribLocationScanline, Scanline);
		glUniform1f(g_AttribLocationBloomRadius, BloomRadius);
		glBindTexture(GL_TEXTURE_2D, g_RenderTexture[SourceTexture]);
		glBindBuffer( GL_ARRAY_BUFFER, glQuadBuffer);
		SetupVertexAttribs(ShaderId::BloomH);
		glDrawArrays(GL_TRIANGLES, 0, 6);
	}

	export void RenderBloomV(RenderTextureId::Value SourceTexture, int width, int height, float Scanline, float BloomRadius)
	{
		glViewport(0, 0, width, height);
		glUseProgram(ShaderTab[ShaderId::BloomV].g_ShaderHandle);
		glUniform1f(ShaderTab[ShaderId::BloomV].g_AttribLocationScanline, Scanline);
		glUniform1f(g_AttribLocationBloomRadius, BloomRadius);
		glBindTexture(GL_TEXTURE_2D, g_RenderTexture[SourceTexture]);
		glBindBuffer( GL_ARRAY_BUFFER, glQuadBuffer);
		SetupVertexAttribs(ShaderId::BloomV);
		glDrawArrays(GL_TRIANGLES, 0, 6);
	}

	export void RenderCRT(RenderTextureId::Value SourceTexture, int width, int height, 
							 float Scanline, 
							 float Roundness, 
							 float BorderSharpness, 
							 float Vignetting, 
							 float Brightness, 
							 float Contrast,
							 float Sharpness,
							 float GridDep,
							 float GhostAmount,
							 float ChromaAmount
							)
	{
		float W = float(width);
		float H = float(height);
		const float Ref = 4.f / 3.f;
		if(W / H > Ref)
			glViewport(GLsizei(.5f*W - .5f*H*Ref), 0, GLsizei(H*Ref), GLsizei(H));
		else
			glViewport(0, GLsizei(.5f*H - .5f*W/Ref), GLsizei(W), GLsizei(W/Ref));

		glUseProgram(ShaderTab[ShaderId::CRT].g_ShaderHandle);

		glUniform1f(g_AttribLocationNbScanlines, 242);
		glUniform1f(g_AttribLocationScanline, Scanline);
		glUniform1f(g_AttribLocationRoundness, Roundness);
		glUniform1f(g_AttribLocationBorderSharpness, BorderSharpness);
		glUniform1f(g_AttribLocationVignetting, Vignetting);
		glUniform1f(g_AttribLocationBrightness, Brightness);
		glUniform1f(g_AttribLocationContrast, Contrast);
		glUniform1f(g_AttribLocationSharpness, Sharpness);
		glUniform1f(g_AttribLocationGridDep, GridDep);
		glUniform1f(g_AttribLocationGhostAmount, GhostAmount);
		glUniform1f(g_AttribLocationChromaAmount, ChromaAmount);

		glBindTexture(GL_TEXTURE_2D, g_RenderTexture[SourceTexture]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glBindBuffer( GL_ARRAY_BUFFER, glQuadBuffer);
		SetupVertexAttribs(ShaderId::CRT);
		glDrawArrays(GL_TRIANGLES, 0, 6);
	}

	export void SetupPostProcessRenderStates()
	{
		glDisable(GL_BLEND);
		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glDepthMask(GL_TRUE);
	}

	export void SetupSolidRender()
	{
		glDisable(GL_BLEND);
		glBlendEquation(GL_FUNC_ADD);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_CULL_FACE);
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_SCISSOR_TEST);
		glCullFace(GL_BACK);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);		
		glDepthFunc(GL_LESS);
		glDepthMask(GL_TRUE);
	}

	export void SetupPrimaryRender(int fb_width, int fb_height, float FOV)
	{
		glViewport(0, 0, (GLsizei)fb_width, (GLsizei)fb_height);
		glUseProgram(ShaderTab[ShaderId::Primary].g_ShaderHandle);
	}
};

/* Per-platform code - gpSP on GCW Zero
 *
 * Copyright (C) 2013 GBATemp user Nebuleon
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public Licens e as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "common.h"
#include <stdarg.h>

#if defined(DINGUX_BETA)
#include <stdlib.h>
#endif

uint32_t PerGameBootFromBIOS;
uint32_t BootFromBIOS;
uint32_t PerGameShowFPS;
uint32_t ShowFPS;
uint32_t PerGameUserFrameskip;
uint32_t UserFrameskip;

uint32_t PerGameIpuKeepAspectRatio;
uint32_t IpuKeepAspectRatio;
uint32_t PerGameIpuFilterType;
uint32_t IpuFilterType;

#define DINGUX_ALLOW_DOWNSCALING_FILE     "/sys/devices/platform/jz-lcd.0/allow_downscaling"
#define DINGUX_KEEP_ASPECT_FILE           "/sys/devices/platform/jz-lcd.0/keep_aspect_ratio"
#define DINGUX_SHARPNESS_UPSCALING_FILE   "/sys/devices/platform/jz-lcd.0/sharpness_upscaling"
#define DINGUX_SHARPNESS_DOWNSCALING_FILE "/sys/devices/platform/jz-lcd.0/sharpness_downscaling"

#define DINGUX_SCALING_MODE_ENVAR         "SDL_VIDEO_KMSDRM_SCALING_MODE"
#define DINGUX_SCALING_SHARPNESS_ENVAR    "SDL_VIDEO_KMSDRM_SCALING_SHARPNESS"

static bool _IpuAllowDownscalingSet = false;
static bool _IpuAllowDownscaling;
static bool _IpuKeepAspectRatioSet = false;
static bool _IpuKeepAspectRatio;
static bool _IpuFilterTypeSet = false;
static enum ipu_filter_type _IpuFilterType;

void SetIpuAllowDownscaling(bool AllowDownscaling)
{
#if !defined(DINGUX_BETA)
	FILE *allow_downscaling_file = NULL;

	if (_IpuAllowDownscalingSet &&
	    (_IpuAllowDownscaling == AllowDownscaling))
	   return;

	allow_downscaling_file = fopen(DINGUX_ALLOW_DOWNSCALING_FILE, "wb");
	if (allow_downscaling_file)
	{
		fputs(AllowDownscaling ? "1" : "0", allow_downscaling_file);
		fclose(allow_downscaling_file);
	}
#endif

	_IpuAllowDownscalingSet = true;
	_IpuAllowDownscaling = AllowDownscaling;
}

void SetIpuKeepAspectRatio(bool KeepAspect)
{
#if !defined(DINGUX_BETA)
	FILE *keep_aspect_file = NULL;
#endif

	if (_IpuKeepAspectRatioSet &&
	    (_IpuKeepAspectRatio == KeepAspect))
	   return;

#if defined(DINGUX_BETA)
   setenv(DINGUX_SCALING_MODE_ENVAR,
			KeepAspect ? "1" : "0", 1);
#else
	keep_aspect_file = fopen(DINGUX_KEEP_ASPECT_FILE, "wb");
	if (keep_aspect_file)
	{
		fputs(KeepAspect ? "1" : "0", keep_aspect_file);
		fclose(keep_aspect_file);
	}
#endif

	_IpuKeepAspectRatioSet = true;
	_IpuKeepAspectRatio = KeepAspect;
}

void SetIpuFilterType(enum ipu_filter_type FilterType)
{
	/* Sharpness settings range is [0,32]
	 * - 0:      nearest-neighbour
	 * - 1:      bilinear
	 * - 2...32: bicubic (translating to a sharpness
	 *                    factor of -0.25..-4.0 internally)
	 * Default bicubic sharpness factor is
	 * (-0.125 * 8) = -1.0 */
	const char *sharpness_str        = "8";
#if !defined(DINGUX_BETA)
	FILE *sharpness_upscaling_file   = NULL;
	FILE *sharpness_downscaling_file = NULL;
#endif

	if (_IpuFilterTypeSet &&
	    (_IpuFilterType == FilterType))
	   return;

	/* Check filter type */
	switch (FilterType)
	{
		case IPU_FILTER_BILINEAR:
			sharpness_str = "1";
			break;
		case IPU_FILTER_NEAREST:
			sharpness_str = "0";
			break;
		case IPU_FILTER_BICUBIC:
		default:
			/* sharpness_str is already set to 8
			 * by default */
			break;
	}

#if defined(DINGUX_BETA)
	setenv(DINGUX_SCALING_SHARPNESS_ENVAR, sharpness_str, 1);
#else
	/* Set upscaling sharpness */
	sharpness_upscaling_file = fopen(DINGUX_SHARPNESS_UPSCALING_FILE, "wb");
	if (sharpness_upscaling_file)
	{
		fputs(sharpness_str, sharpness_upscaling_file);
		fclose(sharpness_upscaling_file);
	}

	/* Set downscaling sharpness */
	sharpness_downscaling_file = fopen(DINGUX_SHARPNESS_DOWNSCALING_FILE, "wb");
	if (sharpness_downscaling_file)
	{
		fputs(sharpness_str, sharpness_downscaling_file);
		fclose(sharpness_downscaling_file);
	}
#endif

	_IpuFilterTypeSet = true;
	_IpuFilterType = FilterType;
}

void ResetIpu(void)
{
#if defined(DINGUX_BETA)
	unsetenv(DINGUX_SCALING_MODE_ENVAR);
	unsetenv(DINGUX_SCALING_SHARPNESS_ENVAR);
#else
	SetIpuKeepAspectRatio(true);
	SetIpuFilterType(IPU_FILTER_BICUBIC);
#endif
}

void ReGBA_Trace(const char* Format, ...)
{
	char* line = malloc(82);
	va_list args;
	int linelen;

	va_start(args, Format);
	if ((linelen = vsnprintf(line, 82, Format, args)) >= 82)
	{
		va_end(args);
		va_start(args, Format);
		free(line);
		line = malloc(linelen + 1);
		vsnprintf(line, linelen + 1, Format, args);
	}
	printf(line);
	va_end(args);
	free(line);
	printf("\r\n");
}

void ReGBA_BadJump(u32 SourcePC, u32 TargetPC)
{
	printf("GBA segmentation fault");
	printf("The game tried to jump from %08X to %08X", SourcePC, TargetPC);
	exit(1);
}

void ReGBA_MaxBlockExitsReached(u32 BlockStartPC, u32 BlockEndPC, u32 Exits)
{
	ReGBA_Trace("Native code exit limit reached");
	ReGBA_Trace("%u exits in the block of GBA code from %08X to %08X", Exits, BlockStartPC, BlockEndPC);
}

void ReGBA_MaxBlockSizeReached(u32 BlockStartPC, u32 BlockEndPC, u32 BlockSize)
{
	ReGBA_Trace("Native code block size reached");
	ReGBA_Trace("%u instructions in the block of GBA code from %08X to %08X", BlockSize, BlockStartPC, BlockEndPC);
}

timespec TimeDifference(timespec Past, timespec Present)
{
	timespec Result;
	Result.tv_sec = Present.tv_sec - Past.tv_sec;

	if (Present.tv_nsec >= Past.tv_nsec)
		Result.tv_nsec = Present.tv_nsec - Past.tv_nsec;
	else
	{
		Result.tv_nsec = 1000000000 - (Past.tv_nsec - Present.tv_nsec);
		Result.tv_sec--;
	}
	return Result;
}

void ReGBA_DisplayFPS(void)
{
	u32 Visible = ResolveSetting(ShowFPS, PerGameShowFPS);
	if (Visible)
	{
		timespec Now;
		clock_gettime(CLOCK_MONOTONIC, &Now);
		timespec Duration = TimeDifference(Stats.LastFPSCalculationTime, Now);
		if (Duration.tv_sec >= 2)
		{
			Visible = 0;
			StatsStopFPS();
			Stats.LastFPSCalculationTime = Now;
		}
		else if (Duration.tv_sec >= 1)
		{
			uint32_t Nanoseconds = (uint32_t) Duration.tv_sec * 1000000000 + Duration.tv_nsec;
			Stats.RenderedFPS = (uint64_t) Stats.RenderedFrames * 1000000000 / Nanoseconds;
			Stats.RenderedFrames = 0;
			Stats.EmulatedFPS = (uint64_t) Stats.EmulatedFrames * 1000000000 / Nanoseconds;
			Stats.EmulatedFrames = 0;
			Stats.LastFPSCalculationTime = Now;
		}
		else
			Visible = Stats.RenderedFPS != -1 && Stats.EmulatedFPS != -1;
	}

	if (Visible)
	{
		char line[512];
		sprintf(line, "%2u/%3u", Stats.RenderedFPS, Stats.EmulatedFPS);
		// White text, black outline
		ScaleModeUnapplied();
		PrintStringOutline(line, RGB888_TO_NATIVE(255, 255, 255), RGB888_TO_NATIVE(0, 0, 0), OutputSurface->pixels, OutputSurface->pitch, 7, 3, OutputSurface->w - 14, OutputSurface->h - 6, LEFT, BOTTOM);
	}
}

void ReGBA_LoadRTCTime(struct ReGBA_RTC* RTCData)
{
	time_t GMTTime = time(NULL);
	struct tm* Time = localtime(&GMTTime);

	RTCData->year = Time->tm_year;
	RTCData->month = Time->tm_mon + 1;
	RTCData->day = Time->tm_mday;
	// Weekday conforms to the expectations (0 = Sunday, 6 = Saturday).
	RTCData->weekday = Time->tm_wday;
	RTCData->hours = Time->tm_hour;
	RTCData->minutes = Time->tm_min;
	RTCData->seconds = Time->tm_sec;
}

bool ReGBA_IsRenderingNextFrame()
{
	uint32_t ResolvedUserFrameskip = ResolveSetting(UserFrameskip, PerGameUserFrameskip);
	if (FastForwardFrameskip != 0) /* fast-forwarding */
	{
		if (ResolvedUserFrameskip != 0)  /* fast-forwarding on user frameskip */
			return FastForwardFrameskipControl == 0 && UserFrameskipControl == 0;
		else /* fast-forwarding on automatic frameskip */
			return FastForwardFrameskipControl == 0 && AudioFrameskipControl == 0;
	}
	else
	{
		if (ResolvedUserFrameskip != 0) /* user frameskip */
			return UserFrameskipControl == 0;
		else /* automatic frameskip */
			return AudioFrameskipControl == 0;
	}
}

const char* GetFileName(const char* Path)
{
	const char* Result = strrchr(Path, '/');
	if (Result)
		return Result + 1;
	return Path;
}

void RemoveExtension(char* Result, const char* FileName)
{
	strcpy(Result, FileName);
	char* Dot = strrchr(Result, '.');
	if (Dot)
		*Dot = '\0';
}

void GetFileNameNoExtension(char* Result, const char* Path)
{
	const char* FileName = GetFileName(Path);
	RemoveExtension(Result, FileName);
}

bool ReGBA_GetBackupFilename(char* Result, const char* GamePath)
{
	char FileNameNoExt[MAX_PATH + 1];
	GetFileNameNoExtension(FileNameNoExt, GamePath);
	if (strlen(main_path) + strlen(FileNameNoExt) + 5 /* / .sav */ > MAX_PATH)
		return false;
	sprintf(Result, "%s/%s.sav", main_path, FileNameNoExt);
	return true;
}

bool ReGBA_GetSavedStateFilename(char* Result, const char* GamePath, uint32_t SlotNumber)
{
	if (SlotNumber >= 100)
		return false;

	char FileNameNoExt[MAX_PATH + 1];
	char SlotNumberString[11];
	GetFileNameNoExtension(FileNameNoExt, GamePath);
	sprintf(SlotNumberString, "%02u", SlotNumber);
	
	if (strlen(main_path) + strlen(FileNameNoExt) + strlen(SlotNumberString) + 2 /* / . */ > MAX_PATH)
		return false;
	sprintf(Result, "%s/%s.s%s", main_path, FileNameNoExt, SlotNumberString);
	return true;
}

bool ReGBA_GetBundledGameConfig(char* Result)
{
	if (executable_path[0] == '\0')
		return false;

	if (strlen(executable_path) + strlen(CONFIG_FILENAME) + 1 /* "/" */ > MAX_PATH)
		return false;

	sprintf(Result, "%s/%s", executable_path, CONFIG_FILENAME);
	return true;
}

void ReGBA_OnGameLoaded(const char* GamePath)
{
}

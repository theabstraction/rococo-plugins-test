#include "RococoTestFPSGameOptions.h"
#include "RococoUE5.h"
#include <CoreMinimal.h>
#include <rococo.great.sex.h>
#include <rococo.strings.h>
#include <rococo.game.options.ex.h>
#include <RococoGuiAPI.h>
#include <GameOptionBuilder.h>
#include <ReflectedGameOptionsBuilder.h>
#include <GameFramework/GameUserSettings.h>
#include <Logging/LogMacros.h>

DECLARE_LOG_CATEGORY_EXTERN(TestFPSOptions, Error, All);
DEFINE_LOG_CATEGORY(TestFPSOptions);

#ifdef _WIN32
#define local_sscanf sscanf_s
#else
#define local_sscanf sscanf
#endif

using namespace Rococo;
using namespace Rococo::GreatSex;
using namespace Rococo::Strings;
using namespace Rococo::Game::Options;

static const char* GEN_HINT_FROM_PARENT_AND_CHOICE = "$*$: ";

namespace Rococo
{
	ROCOCO_API void ConvertFStringToUTF8Buffer(TArray<uint8>& buffer, const FString& src);
}

struct ByWidthAndHeight
{
	bool operator () (const FIntPoint& a, const FIntPoint& b) const
	{
		if (b.X < a.X)
		{
			return true;
		}

		if (b.Y > b.Y)
		{
			return false;
		}

		return b.X < a.X;
	}
};

namespace RococoTestFPS::Implementation
{
	struct AudioOptions : IGameOptions
	{
		OptionDatabase<AudioOptions> db;

		double musicVolume = 25;
		double fxVolume = 20;
		double narrationVolume = 40;
		Rococo::Strings::HString speakerConfig = "2";

		AudioOptions() : db(*this)
		{

		}

		virtual ~AudioOptions()
		{

		}

		IOptionDatabase& DB() override
		{
			return db;
		}

		void Accept(IGameOptionChangeNotifier&) override
		{

		}

		void Revert(IGameOptionChangeNotifier&) override
		{

		}

		bool IsModified() const override
		{
			return true;
		}

		void GetMusicVolume(IScalarInquiry& inquiry)
		{
			inquiry.SetTitle("Music Volume");
			inquiry.SetRange(0, 100.0, 1.0);
			inquiry.SetActiveValue(musicVolume);
			inquiry.SetHint("Set music volume. 0 is off, 100.0 is maximum");
			inquiry.SetDecimalPlaces(0);
		}

		void SetMusicVolume(double value, IGameOptionChangeNotifier&)
		{
			musicVolume = value;
		}

		void GetFXVolume(IScalarInquiry& inquiry)
		{
			inquiry.SetTitle("FX Volume");
			inquiry.SetRange(0, 100.0, 1.0);
			inquiry.SetActiveValue(fxVolume);
			inquiry.SetHint("Set Special FX volume. 0 is off, 100.0 is maximum");
			inquiry.SetDecimalPlaces(0);
		}

		void SetFXVolume(double value, IGameOptionChangeNotifier&)
		{
			fxVolume = value;
		}

		void GetNarrationVolume(IScalarInquiry& inquiry)
		{
			inquiry.SetTitle("Narration Volume");
			inquiry.SetRange(0, 100.0, 1.0);
			inquiry.SetActiveValue(narrationVolume);
			inquiry.SetHint("Set narrator's voice volume. 0 is off, 100.0 is maximum");
			inquiry.SetDecimalPlaces(0);
		}

		void SetNarrationVolume(double value, IGameOptionChangeNotifier&)
		{
			narrationVolume = value;
		}

		void GetSpeakerConfiguration(IChoiceInquiry& inquiry)
		{
			inquiry.SetTitle("Speaker Configuration");
			inquiry.AddChoice("2", "2 (Stereo speakers)", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("2_1", "2.1 (Stereo + Subwoofer)", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("5_1", "5.1 Dolby Surround", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("7_1", "7.1 Dolby Surround", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("2H", "Stereo Headphones", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.SetActiveChoice(speakerConfig);
			inquiry.SetHint("Set sound set-up.");
		}

		void SetSpeakerConfiguration(cstr choice, IGameOptionChangeNotifier&)
		{
			speakerConfig = choice;
		}

		void AddOptions(IGameOptionsBuilder& builder) override
		{
			ADD_GAME_OPTIONS(db, AudioOptions, MusicVolume)
			ADD_GAME_OPTIONS(db, AudioOptions, FXVolume)
			ADD_GAME_OPTIONS(db, AudioOptions, NarrationVolume)
			ADD_GAME_OPTIONS(db, AudioOptions, SpeakerConfiguration)
			db.Build(builder);
		}

		void Refresh(IGameOptionsBuilder& builder) override
		{
			db.Refresh(builder);
		}

		void OnTick(float dt, IGameOptionChangeNotifier&) override
		{
			UNUSED(dt);
		}
	};

	struct NullNotifier : IGameOptionChangeNotifier
	{
		void OnSupervenientOptionChanged(IGameOptions&) override
		{

		}
	} DoNotNotify;

	void SyncViewportToMonitor(const FMonitorInfo& targetMonitor)
	{
		auto& window = *GEngine->GameViewport->GetWindow();

		FVector2D topLeft(targetMonitor.DisplayRect.Left, targetMonitor.DisplayRect.Top);

		enum { LEFT_OFFSET = 32, TOP_OFFSET = 64 };

		UGameUserSettings* settings = GEngine->GetGameUserSettings();
		switch (settings->GetFullscreenMode())
		{
		case EWindowMode::Windowed:
			topLeft += FVector2D(LEFT_OFFSET, TOP_OFFSET);
			break;
		default:
			break;
		}

		window.MoveWindowTo(topLeft);

		FVector2D nativeSpan(targetMonitor.NativeWidth, targetMonitor.NativeHeight);

		switch (settings->GetFullscreenMode())
		{
		case EWindowMode::WindowedFullscreen:
			window.Resize(nativeSpan);
			break;
		case EWindowMode::Fullscreen:
			if (window.GetSizeInScreen().X != nativeSpan.X || window.GetSizeInScreen().Y != nativeSpan.Y)
			{
				window.Resize(nativeSpan);
			}
			break;
		default:
			break;
		}
	}

	[[nodiscard]] bool GetMonitor(int monitorIndex, OUT FMonitorInfo& targetMonitor)
	{
		FDisplayMetrics displayMetrics;
		FDisplayMetrics::RebuildDisplayMetrics(displayMetrics);

		if (displayMetrics.MonitorInfo.IsValidIndex(monitorIndex))
		{
			OUT targetMonitor = displayMetrics.MonitorInfo[monitorIndex];
			return true;
		}

		return false;
	}

	enum { DEFAULT_HREZ = 1920, DEFAULT_VREZ = 1080, DEFAULT_REFRESH_RATE = 60 };

	struct ModeDesc
	{
		cstr shortName;
		cstr desc;
		EWindowMode::Type ue5Type;
	};

	ModeDesc modeDesc[3] = {
		{ "Fullscreen", "Fullscreen", EWindowMode::Fullscreen },
		{ "Windowed", "Windowed", EWindowMode::Windowed },
		{ "FWindowed", "Borderless Windowed", EWindowMode::WindowedFullscreen }
	};

	cstr GetScreenModeChoiceName(EWindowMode::Type type)
	{
		for (auto& desc : modeDesc)
		{
			if (desc.ue5Type == type)
			{
				return desc.shortName;
			}
		}

		return "FWindowed";
	}

	EWindowMode::Type GetScreenModeFromChoiceName(cstr choice)
	{
		for (auto& desc : modeDesc)
		{
			if (Eq(desc.shortName, choice))
			{
				return desc.ue5Type;
			}
		}

		return EWindowMode::WindowedFullscreen;
	}

	enum { MAX_MONITORS = 8 }; // Keep things sane. In 2075, when the average number of monitors on a PC > 8 think about updating this

	// Returns -1 on failure
	int GetMonitorByConfig()
	{
		FDisplayMetrics displayMetrics;
		FDisplayMetrics::RebuildDisplayMetrics(OUT displayMetrics);

		int targetIndex = -1;

		FString configsActiveMonitorName;
		bool hasName = GConfig->GetString(TEXT("/Script/Rococo.Graphics"), TEXT("ActiveMonitorName"), REF configsActiveMonitorName, GGameIni);

		if (hasName)
		{
			for (int i = 0; i < displayMetrics.MonitorInfo.Num() && i < MAX_MONITORS; i++)
			{
				auto& m = displayMetrics.MonitorInfo[i];
				if (m.Name == configsActiveMonitorName)
				{
					if (targetIndex == -1)
					{
						targetIndex = i;
					}
					else
					{
						// Duplicate, so the name does not uniquely identify a monitor, it is of no use to us
						targetIndex = -1;
						break;
					}
				}
			}
		}

		if (targetIndex != -1)
		{
			return targetIndex;
		}

		int configsActiveMonitorIndex;
		bool hasIndex = GConfig->GetInt(TEXT("/Script/Rococo.Graphics"), TEXT("ActiveMonitorIndex"), REF configsActiveMonitorIndex, GGameIni);

		if (hasIndex)
		{
			if (displayMetrics.MonitorInfo.IsValidIndex(configsActiveMonitorIndex))
			{
				return configsActiveMonitorIndex;
			}
		}

		return -1;
	}

	void SetRezInConfig(int width, int height)
	{
		GConfig->SetInt(TEXT("/Script/Rococo.Graphics"), TEXT("FullScreenResolution.Width"), width, GGameIni);
		GConfig->SetInt(TEXT("/Script/Rococo.Graphics"), TEXT("FullScreenResolution.Height"), height, GGameIni);
	}

	int ClampMaxFramerate(int hz)
	{
		return clamp(hz, 60, 200);
	}

	Vec2i GetRezFromConfig()
	{
		int configWidth = 0, configHeight = 0;
		GConfig->GetInt(TEXT("/Script/Rococo.Graphics"), TEXT("FullScreenResolution.Width"), OUT configWidth, GGameIni);
		GConfig->GetInt(TEXT("/Script/Rococo.Graphics"), TEXT("FullScreenResolution.Height"), OUT configHeight, GGameIni);
		return { configWidth, configHeight };
	}

	Vec2i GetRezFromSettings()
	{
		UGameUserSettings* settings = GEngine->GetGameUserSettings();
		FIntPoint resolution = settings->GetScreenResolution();

		return { resolution.X, resolution.Y };
	}

	int GetRefreshRateFromSettings()
	{
		int hz;
		if (!GConfig->GetInt(TEXT("/Script/Rococo.Graphics"), TEXT("FullScreenResolution.Hz"), OUT hz, GGameIni))
		{
			hz = DEFAULT_REFRESH_RATE;
		}

		return ClampMaxFramerate(hz);
	}

	struct GraphicsOptions : IGameOptions
	{
		OptionDatabase<GraphicsOptions> db;

		virtual ~GraphicsOptions()
		{

		}

		IOptionDatabase& DB() override
		{
			return db;
		}

		EWindowMode::Type GetFullscreenModeFromSettings() const
		{
			UGameUserSettings* settings = GEngine->GetGameUserSettings();
			auto mode = settings->GetFullscreenMode();
			return mode;
		}

		void SetFullscreenModeInSettings(EWindowMode::Type type)
		{
			UGameUserSettings* settings = GEngine->GetGameUserSettings();
			settings->SetFullscreenMode(type);
		}

		void UpdateScreen(IGameOptionChangeNotifier& notifier)
		{
			FMonitorInfo targetMonitor;
			if (TryGetCurrentTargetMonitor(OUT targetMonitor))
			{
				auto currentMode = GetFullscreenModeFromSettings();
				if (currentMode == EWindowMode::Fullscreen)
				{
					// We need to switch out of full screen to change monitor
					SetFullscreenModeInSettings(EWindowMode::WindowedFullscreen);
					SyncVideoModeToCurrentTargetResolution(&targetMonitor);
				}

				// Push the screen onto the target monitor. This is half-arsed because Unreal Engine does little to expose the DX12 screen config API for monitor micromanagement
				SyncViewportToMonitor(targetMonitor);

				if (currentMode == EWindowMode::Fullscreen)
				{
					// Switch back to full screen now that we have shifted the screen to the other monitor
					SetFullscreenModeInSettings(EWindowMode::Fullscreen);
				}

				SyncVideoModeToCurrentTargetResolution(&targetMonitor);
			}

			notifier.OnSupervenientOptionChanged(*this);
		}

		void Accept(IGameOptionChangeNotifier& notifier) override
		{
			UpdateScreen(notifier);
		}

		void Revert(IGameOptionChangeNotifier& notifier) override
		{
			targetMonitorIndex = 0;
			SetFullscreenModeInSettings(EWindowMode::WindowedFullscreen);
			SetRezInConfig(DEFAULT_HREZ, DEFAULT_VREZ);
			GConfig->SetInt(TEXT("/Script/Rococo.Graphics"), TEXT("FullScreenResolution.Hz"), DEFAULT_REFRESH_RATE, GGameIni);

			UpdateScreen(notifier);
		}

		bool IsModified() const override
		{
			return true;
		}

		Rococo::Strings::HString shadowQuality = "2";
		Rococo::Strings::HString landscapeQuality = "1";
		Rococo::Strings::HString reflectionAlgorithm = "1";
		Rococo::Strings::HString textureQuality = "1";
		Rococo::Strings::HString waterQuality = "1";
		bool isFSAAEnabled = false;

		GraphicsOptions() : db(*this)
		{
			
		}

		void GetScreenMode(IChoiceInquiry& inquiry)
		{
			inquiry.SetTitle("Screen Mode");

			for (auto& mode : modeDesc)
			{
				inquiry.AddChoice(mode.shortName, mode.desc, GEN_HINT_FROM_PARENT_AND_CHOICE);
			}

			inquiry.SetHint("Set screen mode");

			auto currentMode = GetFullscreenModeFromSettings();
			cstr choice = GetScreenModeChoiceName(currentMode);
			inquiry.SetActiveChoice(choice);
		}

		void SetScreenMode(cstr choice, IGameOptionChangeNotifier& notifier)
		{
			auto mode = GetScreenModeFromChoiceName(choice);
			SetFullscreenModeInSettings(mode);
			UpdateScreen(notifier);
		}

		void GetFSAA(IBoolInquiry& inquiry)
		{
			inquiry.SetTitle("Fullscreen Anti-Aliasing (N/A)");
			inquiry.SetActiveValue(isFSAAEnabled);
			inquiry.SetHint("Enable or disable full screen anti-aliasing");
		}

		void SetFSAA(bool value, IGameOptionChangeNotifier&)
		{
			isFSAAEnabled = value;
		}

		void GetShadowQuality(IChoiceInquiry& inquiry)
		{
			inquiry.SetTitle("Shadow Quality (N/A)");
			inquiry.AddChoice("1", "512 x 512 1pt (low)", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("2", "1024 x 1024 4pt (medium)", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("3", "1024 x 1024 16pt (high)", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("4", "2048 x 2048 4pt (very high)", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("5", "2048 x 2048 16pt (ultra)", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.SetActiveChoice(shadowQuality);
			inquiry.SetHint("Set the quality of shadows. Higher settings may reduce frame-rate");
		}

		void SetShadowQuality(cstr value, IGameOptionChangeNotifier&)
		{
			shadowQuality = value;
		}

		void GetLandscapeQuality(IChoiceInquiry& inquiry)
		{
			inquiry.SetTitle("Landscape Quality (N/A)");
			inquiry.AddChoice("1", "Low", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("2", "Medium", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("3", "High", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("4", "Ultra", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("5", "Ultra (Experimental)", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.SetActiveChoice(landscapeQuality);
			inquiry.SetHint("Set the quality of landscape rendering. Higher settings may reduce frame-rate");
		}

		void SetLandscapeQuality(cstr value, IGameOptionChangeNotifier&)
		{
			landscapeQuality = value;
		}

		void GetReflectionAlgorithm(IChoiceInquiry& inquiry)
		{
			inquiry.SetTitle("Reflection Algorithm (N/A)");
			inquiry.AddChoice("1", "Gloss (Minimal)", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("2", "Global Environmental Mapping (Low)", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("3", "Local Environmental Mapping (Medium)", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("4", "Dynamic Environmental Mapping (High)", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("5", "Raytracing (Ultra)", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.SetActiveChoice(reflectionAlgorithm);
			inquiry.SetHint("Set the quality of reflections. Higher settings may reduce frame-rate");
		}

		void SetReflectionAlgorithm(cstr value, IGameOptionChangeNotifier&)
		{
			reflectionAlgorithm = value;
		}

		int targetMonitorIndex = -1;


		// Initialize the targetMonitorIndex value
		void InitTargetMonitorIndex()
		{
			if (targetMonitorIndex == -1)
			{
				targetMonitorIndex = GetMonitorByConfig();
				if (targetMonitorIndex >= 0)
				{
					return;
				}

				FDisplayMetrics displayMetrics;
				FDisplayMetrics::RebuildDisplayMetrics(OUT displayMetrics);

				auto& window = *GEngine->GameViewport->GetWindow();

				for (int i = 0; i < displayMetrics.MonitorInfo.Num(); i++)
				{
					const FMonitorInfo& m = displayMetrics.MonitorInfo[i];
					if (window.GetPositionInScreen() == FVector2D(m.WorkArea.Left, m.WorkArea.Top))
					{
						targetMonitorIndex = i;
						return;
					}
				}

				for (int i = 0; i < displayMetrics.MonitorInfo.Num(); i++)
				{
					const FMonitorInfo& m = displayMetrics.MonitorInfo[i];
					if (m.bIsPrimary)
					{
						targetMonitorIndex = i;
						return;
					}
				}

				targetMonitorIndex = 0;
			}
		}

		void GetActiveMonitor(IChoiceInquiry& inquiry)
		{
			inquiry.SetTitle("Active Monitor");
			inquiry.SetHint("Set which monitor to present the game in fullscreen");

			FDisplayMetrics displayMetrics;
			FDisplayMetrics::RebuildDisplayMetrics(displayMetrics);

			enum { MAX_MONITORS = 8 };

			for (int i = 0; i < displayMetrics.MonitorInfo.Num() && i < MAX_MONITORS; i++)
			{
				auto& m = displayMetrics.MonitorInfo[i];

				char choiceName[8];
				SafeFormat(choiceName, "%d", i + 1);

				TArray<uint8> buffer;
				Rococo::ConvertFStringToUTF8Buffer(buffer, m.Name);

				char desc[64];
				SafeFormat(desc, "%d - %dx%d [%s]", i + 1, m.NativeWidth, m.NativeHeight, buffer.GetData());

				inquiry.AddChoice(choiceName, desc, GEN_HINT_FROM_PARENT_AND_CHOICE);
			}

			InitTargetMonitorIndex();

			char choice[8];
			SafeFormat(choice, "%d", targetMonitorIndex + 1);
			inquiry.SetActiveChoice(choice);
		}

		void SanitizeTargetResolutions(const FMonitorInfo& selectedMonitor)
		{
			if (selectedMonitor.NativeWidth < (int)currentTargetResolution.Width || selectedMonitor.NativeHeight < (int)currentTargetResolution.Height || currentTargetResolution.Width < minHRez || currentTargetResolution.Height < minVRez)
			{
				currentTargetResolution.Width = selectedMonitor.NativeWidth;
				currentTargetResolution.Height = selectedMonitor.NativeHeight;
			}
		}

		void SyncMonitorToIndexAndUpdateScreen(int monitorIndex, bool saveToConfig, IGameOptionChangeNotifier& notifier)
		{
			FMonitorInfo targetMonitor;
			if (!GetMonitor(monitorIndex, OUT targetMonitor))
			{
				return;
			}

			if (saveToConfig)
			{
				GConfig->SetString(TEXT("/Script/Rococo.Graphics"), TEXT("ActiveMonitorName"), *targetMonitor.Name, GGameIni);
				GConfig->SetInt(TEXT("/Script/Rococo.Graphics"), TEXT("ActiveMonitorIndex"), monitorIndex, GGameIni);
				GConfig->Flush(false, GGameIni);
			}

			UpdateScreen(notifier);
		}

		void MaximizeResolution(int monitorIndex)
		{
			FMonitorInfo targetMonitor;
			if (!GetMonitor(monitorIndex, OUT targetMonitor))
			{
				return;
			}

			currentTargetResolution.Width = targetMonitor.NativeWidth;
			currentTargetResolution.Height = targetMonitor.NativeHeight;

			SetRezInConfig(currentTargetResolution.Width, currentTargetResolution.Height);
		}

		void SetMaximumResolutionIfScreenSpansMismatch(int lastIndex, int newIndex)
		{
			if (lastIndex == newIndex)
			{
				return;
			}

			FMonitorInfo oldMonitor;
			FMonitorInfo newMonitor;

			bool resetFullsizeSpan = false;

			if (GetMonitor(lastIndex, OUT oldMonitor) && GetMonitor(newIndex, OUT newMonitor))
			{
				resetFullsizeSpan = oldMonitor.NativeWidth != newMonitor.NativeWidth || oldMonitor.NativeHeight != newMonitor.NativeHeight;
			}
			else
			{
				resetFullsizeSpan = true;
			}

			if (resetFullsizeSpan)
			{
				MaximizeResolution(newIndex);
			}
		}

		void SetActiveMonitor(cstr value, IGameOptionChangeNotifier& notifier)
		{
			int lastIndex = targetMonitorIndex;
			// Assume values is "1", "2", etc, a positive integer
			targetMonitorIndex = atoi(value) - 1;

			SyncMonitorToIndexAndUpdateScreen(targetMonitorIndex, true, DoNotNotify);
			SetMaximumResolutionIfScreenSpansMismatch(lastIndex, targetMonitorIndex);
			SyncMonitorToIndexAndUpdateScreen(targetMonitorIndex, true, notifier);
		}

		bool TrySetCurrentRezAndMaxRefreshRateTo(int width, int height, int hz, const TArray<FIntPoint>& knownResolutions, REF IChoiceInquiry& inquiry)
		{
			int index = 0;
			for (auto& r : knownResolutions)
			{
				if (r.X == width && r.Y == height)
				{
					currentTargetResolution.Width = width;
					currentTargetResolution.Height = height;

					SetRezInConfig(width, height);

					char choiceName[16];
					SafeFormat(choiceName, "%d %d %d", index + 1, r.X, r.Y);
					inquiry.SetActiveChoice(choiceName);
					return true;
				}

				index++;
			}

			return false;
		}

		void ComputeDefaultResolution(const TArray<FIntPoint>& knownResolutions, REF IChoiceInquiry& inquiry)
		{
			FDisplayMetrics m;
			FDisplayMetrics::RebuildDisplayMetrics(OUT m);

			currentTargetResolution.Width = m.PrimaryDisplayWidth;
			currentTargetResolution.Height = m.PrimaryDisplayHeight;
			currentTargetResolution.RefreshRate = 60;

			if (TrySetCurrentRezAndMaxRefreshRateTo(m.PrimaryDisplayWidth, m.PrimaryDisplayHeight, DEFAULT_REFRESH_RATE, knownResolutions, inquiry))
			{
				return;
			}

			if (TrySetCurrentRezAndMaxRefreshRateTo(DEFAULT_HREZ, DEFAULT_VREZ, DEFAULT_REFRESH_RATE, knownResolutions, inquiry))
			{
				return;
			}

			char defaultText[32];
			SafeFormat(defaultText, "%d x %d", DEFAULT_HREZ, DEFAULT_VREZ);

			cstr defaultChoice = "default";
			inquiry.AddChoice(defaultChoice, defaultText, GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.SetActiveChoice(defaultChoice);
		}

		void GetMaximumFramerate(IChoiceInquiry& inquiry)
		{
			inquiry.SetTitle("Maximum Framerate");
			inquiry.SetHint("Set fullscreen maximum framerate");
			inquiry.AddChoice("60", "60 Hz", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("100", "100 Hz", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("120", "120 Hz", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("144", "144 Hz", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("180", "180 Hz", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("200", "200 Hz", GEN_HINT_FROM_PARENT_AND_CHOICE);

			int hz = GetRefreshRateFromSettings();
			
			char configChoice[8];
			SafeFormat(configChoice, "%d", hz);
			inquiry.SetActiveChoice(configChoice);
		}

		void SetMaximumFramerate(cstr choice, IGameOptionChangeNotifier& notifier)
		{
			currentTargetResolution.RefreshRate = ClampMaxFramerate(atoi(choice));
			GConfig->SetInt(TEXT("/Script/Rococo.Graphics"), TEXT("FullScreenResolution.Hz"), currentTargetResolution.RefreshRate, GGameIni);
			UpdateScreen(notifier);
		}

		bool BuildSortedUniqueSysResolutions()
		{
			resolutions.Empty();

			TArray<FScreenResolutionRHI> sysResolutions;
			if (RHIGetAvailableResolutions(sysResolutions, true))
			{
				for (auto& r : sysResolutions)
				{
					FIntPoint span(r.Width, r.Height);
					resolutions.AddUnique(span);
				}

				ByWidthAndHeight byWidthAndHeight;
				resolutions.Sort(byWidthAndHeight);
				return true;
			}

			return false;
		}

		FScreenResolutionRHI currentTargetResolution { DEFAULT_HREZ, DEFAULT_VREZ, DEFAULT_REFRESH_RATE };
		TArray<FIntPoint> resolutions;

		enum { minHRez = 1280, minVRez = 768 };

		void GetFullscreenResolution(IChoiceInquiry& inquiry)
		{
			inquiry.SetHint("Set full screen resolution");
			inquiry.SetTitle("Fullscreen Resolution");

			
			int currentIndex = -1;

			if (BuildSortedUniqueSysResolutions())
			{	
				Vec2i configSpan = GetRezFromConfig();

				int index = 0;
				for (const FIntPoint& r : resolutions)
				{
					if (r.X > minHRez && r.Y > minVRez)
					{
						if (r.X == configSpan.x && r.Y == configSpan.y)
						{
							currentIndex = index;
						}

						char choiceName[16];
						SafeFormat(choiceName, "%d %d %d", index, r.X, r.Y);

						char choiceDesc[32];
						SafeFormat(choiceDesc, "%d x %d", r.X, r.Y);

						inquiry.AddChoice(choiceName, choiceDesc, GEN_HINT_FROM_PARENT_AND_CHOICE);

						index++;
					}					
				}
			}

			if (currentIndex == -1)
			{
				ComputeDefaultResolution(IN resolutions, REF inquiry);
			}
			else
			{
				auto& r = resolutions[currentIndex];

				char choiceName[16];
				SafeFormat(choiceName, "%d %d %d", currentIndex, r.X, r.Y);
				inquiry.SetActiveChoice(choiceName);
			}
		}

		[[nodiscard]] bool TryGetCurrentTargetMonitor(OUT FMonitorInfo& currentMonitor)
		{
			FDisplayMetrics displayMetrics;
			FDisplayMetrics::RebuildDisplayMetrics(displayMetrics);

			auto& window = *GEngine->GameViewport->GetWindow();

			if (displayMetrics.MonitorInfo.IsValidIndex(targetMonitorIndex))
			{
				OUT currentMonitor = displayMetrics.MonitorInfo[targetMonitorIndex];
				return true;
			}

			for (const FMonitorInfo& m : displayMetrics.MonitorInfo)
			{
				FVector2D leftTop = window.GetPositionInScreen();
				if (leftTop.X >= m.WorkArea.Left && leftTop.X < m.WorkArea.Right)
				{
					if (leftTop.Y >= m.WorkArea.Top && leftTop.Y < m.WorkArea.Bottom)
					{
						currentMonitor = m;
						return true;
					}
				}
			}

			for (const FMonitorInfo& m : displayMetrics.MonitorInfo)
			{
				if (m.bIsPrimary)
				{
					currentMonitor = m;
					return true;
				}
			}

			return false;
		}

		void SetFullscreenResolution(cstr choice, IGameOptionChangeNotifier& notifier)
		{
			currentTargetResolution = FScreenResolutionRHI(DEFAULT_HREZ, DEFAULT_VREZ, DEFAULT_REFRESH_RATE);

			if (!Eq(choice, "default"))
			{
				int index;
				int width;
				int height;

				if (3 == local_sscanf(choice, "%d%d%d", &index, &width, &height))
				{
					if (resolutions.IsValidIndex(index))
					{
						const auto& r = resolutions[index];
						if (r.X == width && r.Y == height)
						{
							currentTargetResolution.Width = width;
							currentTargetResolution.Height = height;
						}
					}
				}
			}
			else
			{
				// In the case of "default" or no match, we use the default values at the top of the function
			}

			SetRezInConfig(currentTargetResolution.Width, currentTargetResolution.Height);

			UpdateScreen(notifier);			
		}

		void SyncVideoModeToCurrentTargetResolution(const FMonitorInfo* selectedMonitor)
		{
			if (selectedMonitor != nullptr)
			{
				SanitizeTargetResolutions(*selectedMonitor);
			}			

			UGameUserSettings* settings = GEngine->GetGameUserSettings();

			FIntPoint nextRez(currentTargetResolution.Width, currentTargetResolution.Height);

			auto modality = settings->GetFullscreenMode();

			if (modality == EWindowMode::Windowed)
			{
				enum 
				{
					FULLSCREEN_TO_WINDOW_TRUNCATION_WIDTH = 384,
					FULLSCREEN_TO_WINDOW_TRUNCATION_HEIGHT = 216
				};

				nextRez.X -= FULLSCREEN_TO_WINDOW_TRUNCATION_WIDTH;
				nextRez.Y -= FULLSCREEN_TO_WINDOW_TRUNCATION_HEIGHT;
			}

			settings->SetScreenResolution(nextRez);
			settings->SetFrameRateLimit(currentTargetResolution.RefreshRate);
			settings->SetFullscreenMode(modality);
			settings->ApplySettings(true);
			settings->ConfirmVideoMode();
			settings->RequestUIUpdate();
		}

		void GetTextureQuality(IChoiceInquiry& inquiry)
		{
			inquiry.SetTitle("Texture Quality (N/A)");
			inquiry.AddChoice("1", "Low 256x256", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("2", "Medium 512x512", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("3", "High 1024x1024 ", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("4", "Very High 2048x2048", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("5", "Ultra 4096x2096", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.SetActiveChoice(textureQuality);
			inquiry.SetHint("Set the texture sizes. Higher settings may slow frame rate");
		}

		void SetTextureQuality(cstr choice, IGameOptionChangeNotifier&)
		{
			textureQuality = choice;
		}

		void GetWaterQuality(IChoiceInquiry& inquiry)
		{
			inquiry.SetTitle("Water Quality (N/A)");
			inquiry.AddChoice("1", "Low - flat", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("2", "Medium - ripples", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("3", "High - ripples & refraction", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("4", "Very High - full physics", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("5", "Ultra - physics and reflections", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.SetActiveChoice(waterQuality);
			inquiry.SetHint("Set the quality of water rendering. Higher settings may slow frame rate");
		}

		void SetWaterQuality(cstr choice, IGameOptionChangeNotifier&)
		{
			waterQuality = choice;
		}

		void AddOptions(IGameOptionsBuilder& builder) override
		{
			ADD_GAME_OPTIONS(db, GraphicsOptions, ActiveMonitor)
			ADD_GAME_OPTIONS(db, GraphicsOptions, ScreenMode)
			ADD_GAME_OPTIONS(db, GraphicsOptions, FullscreenResolution)
			ADD_GAME_OPTIONS(db, GraphicsOptions, MaximumFramerate)
			ADD_GAME_OPTIONS(db, GraphicsOptions, FSAA)
			ADD_GAME_OPTIONS(db, GraphicsOptions, ShadowQuality)
			ADD_GAME_OPTIONS(db, GraphicsOptions, LandscapeQuality)
			ADD_GAME_OPTIONS(db, GraphicsOptions, ReflectionAlgorithm)
			ADD_GAME_OPTIONS(db, GraphicsOptions, TextureQuality)
			ADD_GAME_OPTIONS(db, GraphicsOptions, WaterQuality)
			db.Build(builder);
		}

		void Refresh(IGameOptionsBuilder& builder) override
		{
			db.Refresh(builder);
		}

		void OnTick(float dt, IGameOptionChangeNotifier&) override
		{
			UNUSED(dt);
		}

		void SyncGraphicsToConfig()
		{
			if (!GEngine->GameViewport || !GEngine->GameViewport->GetWindow())
			{
				UE_LOG(TestFPSOptions, Error, TEXT("%hs: viewport window does not exist at time of invocation"), __func__);
				return;
			}

			GEngine->GameViewport->GetWindow()->SetViewportSizeDrivenByWindow(true);

			Vec2i res = GetRezFromConfig();
	
			currentTargetResolution.Width = res.x;;
			currentTargetResolution.Height = res.y;

			int hz = GetRefreshRateFromSettings();
			currentTargetResolution.RefreshRate = hz;

			InitTargetMonitorIndex();

			// Config has determined our index, so we do not have to save it
			SyncMonitorToIndexAndUpdateScreen(targetMonitorIndex, /* saveToConfig */ false, DoNotNotify);
		}
	};

	struct UIOptions : IGameOptions
	{
		OptionDatabase<UIOptions> db;

		double cursorResponsiveness = 3.0;
		bool isYAxisInverted = false;

		UIOptions() : db(*this)
		{

		}

		virtual ~UIOptions()
		{

		}

		IOptionDatabase& DB() override
		{
			return db;
		}

		void Accept(IGameOptionChangeNotifier&) override
		{

		}

		void Revert(IGameOptionChangeNotifier&) override
		{

		}

		bool IsModified() const override
		{
			return true;
		}

		void GetCursorResponsiveness(IScalarInquiry& inquiry)
		{
			inquiry.SetTitle("Mouse Sensitivity");
			inquiry.SetRange(1, 10, 1);
			inquiry.SetActiveValue(cursorResponsiveness);
			inquiry.SetHint("Set scaling of mouse movement to cursor movement");
		}

		void SetCursorResponsiveness(double value, IGameOptionChangeNotifier&)
		{
			cursorResponsiveness = value;
		}

		void GetInvertYAxis(IBoolInquiry& inquiry)
		{
			inquiry.SetTitle("Invert Y-Axis");
			inquiry.SetActiveValue(isYAxisInverted);
			inquiry.SetHint("Reverse the response to player ascent to the joystick direction");
		}

		void SetInvertYAxis(bool value, IGameOptionChangeNotifier&)
		{
			isYAxisInverted = value;
		}

		void AddOptions(IGameOptionsBuilder& builder) override
		{
			ADD_GAME_OPTIONS(db, UIOptions, CursorResponsiveness)
			ADD_GAME_OPTIONS(db, UIOptions, InvertYAxis)
			db.Build(builder);
		}

		void Refresh(IGameOptionsBuilder& builder) override
		{
			db.Refresh(builder);
		}

		void OnTick(float dt, IGameOptionChangeNotifier&) override
		{
			UNUSED(dt);
		}
	};

	struct GameplayOptions : IGameOptions
	{
		OptionDatabase<GameplayOptions> db;

		Rococo::Strings::HString startDifficulty = "Easy";
		Rococo::Strings::HString gameDifficulty = "Easy";
		Rococo::Strings::HString playerName = "Geoff";

		GameplayOptions() : db(*this)
		{

		}

		virtual ~GameplayOptions()
		{

		}

		IOptionDatabase& DB() override
		{
			return db;
		}

		void Accept(IGameOptionChangeNotifier&) override
		{

		}

		void Revert(IGameOptionChangeNotifier&) override
		{

		}

		bool IsModified() const override
		{
			return true;
		}

		void GetStartingDifficulty(IChoiceInquiry& inquiry)
		{
			inquiry.SetTitle("Starting Difficulty");
			inquiry.AddChoice("Easy", "Easy", "Recruits cannot die during training");
			inquiry.AddChoice("Medium", "Medium", "Recruits have three lives to complete training");
			inquiry.AddChoice("Hard", "Realistic", "Recruits can die during training");
			inquiry.AddChoice("Ironman", "Ironman", "No save game slots during training");

			inquiry.SetActiveChoice(startDifficulty);
			inquiry.SetHint("Set the difficulty of the training mission");
		}

		void SetStartingDifficulty(cstr value, IGameOptionChangeNotifier&)
		{
			startDifficulty = value;
		}

		void GetGameDifficulty(IChoiceInquiry& inquiry)
		{
			inquiry.SetTitle("Game Difficulty");
			inquiry.AddChoice("Easy", "Easy", "On death you respawn at the last checkpoint");
			inquiry.AddChoice("Medium", "Medium", "You respawn on death, but lose 25% xp");
			inquiry.AddChoice("Hard", "Realistic", "No respawns on death");
			inquiry.AddChoice("Ironman", "Ironman", "Save game deleted on death... and no respawn on death");

			inquiry.SetActiveChoice(gameDifficulty);
			inquiry.SetHint("Set the difficulty of the main game");
		}

		void SetGameDifficulty(cstr value, IGameOptionChangeNotifier&)
		{
			gameDifficulty = value;
		}

		void SetPlayerName(cstr value, IGameOptionChangeNotifier&)
		{
			if (*value == 0)
			{
				playerName = "Geoff";
				return;
			}

			playerName = value;
		}

		void GetPlayerName(IStringInquiry& inquiry)
		{
			inquiry.SetTitle("Player Name");
			inquiry.SetActiveValue(playerName);
			inquiry.SetHint("Set the name of your player's avatar");
		}

		void AddOptions(IGameOptionsBuilder& builder) override
		{
			ADD_GAME_OPTIONS(db, GameplayOptions, StartingDifficulty)
			ADD_GAME_OPTIONS(db, GameplayOptions, GameDifficulty)
			ADD_GAME_OPTIONS_STRING(db, GameplayOptions, PlayerName, 32)

			db.Build(builder);
		}

		void Refresh(IGameOptionsBuilder& builder) override
		{
			db.Refresh(builder);
		}

		void OnTick(float dt, IGameOptionChangeNotifier&) override
		{
			UNUSED(dt);
		}
	};

	struct MultiplayerOptions : IGameOptions
	{
		OptionDatabase<MultiplayerOptions> db;

		bool hostGame = false;
		bool useUDP = true;

		MultiplayerOptions() : db(*this)
		{

		}

		virtual ~MultiplayerOptions()
		{

		}

		IOptionDatabase& DB() override
		{
			return db;
		}

		void Accept(IGameOptionChangeNotifier&) override
		{

		}

		void Revert(IGameOptionChangeNotifier&) override
		{

		}

		bool IsModified() const override
		{
			return true;
		}

		void GetHostGame(IBoolInquiry& inquiry)
		{
			inquiry.SetTitle("Host Game");
			inquiry.SetActiveValue(hostGame);
		}

		void SetHostGame(bool value, IGameOptionChangeNotifier&)
		{
			hostGame = value;
		}

		void GetUseUDP(IBoolInquiry& inquiry)
		{
			inquiry.SetTitle("UDP");
			inquiry.SetActiveValue(useUDP);
		}

		void SetUseUDP(bool value, IGameOptionChangeNotifier&)
		{
			useUDP = value;
		}

		void AddOptions(IGameOptionsBuilder& builder) override
		{
			ADD_GAME_OPTIONS(db, MultiplayerOptions, HostGame)
			ADD_GAME_OPTIONS(db, MultiplayerOptions, UseUDP)
			db.Build(builder);
		}

		void Refresh(IGameOptionsBuilder& builder) override
		{
			db.Refresh(builder);
		}

		void OnTick(float dt, IGameOptionChangeNotifier&) override
		{
			UNUSED(dt);
		}
	};

	GraphicsOptions s_GraphicsOptions;
	AudioOptions s_AudioOptions;
	UIOptions s_UIOptions;
	GameplayOptions s_gameplayOptions;
	MultiplayerOptions s_MultiplayerOptions;
}

namespace RococoTestFPS
{
	void PrepGenerator(IReflectedGameOptionsBuilder& optionsBuilder, const TArray<UObject*>& context, Rococo::GreatSex::IGreatSexGenerator& generator)
	{
		using namespace RococoTestFPS::Implementation;

		generator.AddOptions(s_GraphicsOptions, "GraphicsOptions");
		generator.AddOptions(s_AudioOptions, "AudioOptions");
		generator.AddOptions(s_UIOptions, "UIOptions");
		generator.AddOptions(s_gameplayOptions, "GameplayOptions");
		generator.AddOptions(s_MultiplayerOptions, "MultiplayerOptions");

		for (auto* object : context)
		{
			if (object && object->Implements<URococoGameOptionBuilder>())
			{
				auto builder = TScriptInterface<IRococoGameOptionBuilder>(object);
				FString optionCategory = builder->Execute_GetOptionId(object);
				optionsBuilder.ReflectIntoGenerator(*object, optionCategory, generator);
			}
		}
	}

	void InitGlobalOptions()
	{
		Rococo::Gui::SetGlobalPrepGenerator(PrepGenerator);
	}
}

double URococoTestFPSGameOptionsLibrary::GetMusicVolume()
{
	return RococoTestFPS::Implementation::s_AudioOptions.musicVolume / 100.0;
}

double URococoTestFPSGameOptionsLibrary::GetNarrationVolume()
{
	return RococoTestFPS::Implementation::s_AudioOptions.narrationVolume / 100.0;
}

double URococoTestFPSGameOptionsLibrary::GetFXVolume()
{
	return RococoTestFPS::Implementation::s_AudioOptions.fxVolume / 100.0;
}

void URococoTestFPSGameOptionsLibrary::SyncGraphicsToConfig()
{
	RococoTestFPS::Implementation::s_GraphicsOptions.SyncGraphicsToConfig();
}
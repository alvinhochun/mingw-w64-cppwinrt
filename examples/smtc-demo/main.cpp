/*
 * SPDX-FileCopyrightText: 2022 Alvin Wong
 *
 * SPDX-License-Identifier: MIT
 */

#include <array>
#include <chrono>
#include <mutex>
#include <queue>
#include <variant>

#include <cstdio>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Media.h>
#include <winrt/Windows.Media.Playback.h>

using winrt::Windows::Foundation::TimeSpan;
using winrt::Windows::Media::MediaPlaybackAutoRepeatMode;
using winrt::Windows::Media::MediaPlaybackStatus;
using winrt::Windows::Media::SystemMediaTransportControls;
using winrt::Windows::Media::SystemMediaTransportControlsButton;
using winrt::Windows::Media::SystemMediaTransportControlsButtonPressedEventArgs;
using winrt::Windows::Media::SystemMediaTransportControlsTimelineProperties;
using winrt::Windows::Media::Playback::MediaPlayer;


struct song_info {
    const wchar_t *title;
    const wchar_t *artist;
    int lengthSecs;
};

const std::array<song_info, 3> s_songList = {{
    song_info {
        .title = L"Never Gonna Give You Up",
        .artist = L"Rick Astley",
        .lengthSecs = 3 * 60 + 33,
    },
    song_info {
        .title = L"Gangnam Style",
        .artist = L"Psy",
        .lengthSecs = 4 * 60 + 12,
    },
    song_info {
        .title = L"Despacito",
        .artist = L"Luis Fonsi",
        .lengthSecs = 4 * 60 + 41,
    },
}};

struct button_command {
    SystemMediaTransportControlsButton m_button;
};

struct seek_command {
    int m_seekPos;
};

MediaPlayer s_mediaPlayer{ nullptr };
std::queue<std::variant<button_command, seek_command>> s_queue;
std::mutex s_queueMutex;
HANDLE s_queueEvent;
int s_currentSongIdx{ 0 };
int s_currentPlayPos{ 0 };
bool s_isPlaying{ false };
HANDLE s_mockPlayerTimer;

void updateSongInfo(const SystemMediaTransportControls &smtc) {
    const auto &currentSong = s_songList[s_currentSongIdx];
    auto updater = smtc.DisplayUpdater();
    updater.Type(winrt::Windows::Media::MediaPlaybackType::Music);
    updater.MusicProperties().Title(currentSong.title);
    updater.MusicProperties().Artist(currentSong.artist);
    updater.Update();
    wprintf(L"Now playing: %s\n", currentSong.title);
}

void updatePlaybackStatus(const SystemMediaTransportControls &smtc) {
    smtc.PlaybackStatus(s_isPlaying ? MediaPlaybackStatus::Playing : MediaPlaybackStatus::Paused);
}

void updateTimeline(const SystemMediaTransportControls &smtc) {
    static SystemMediaTransportControlsTimelineProperties timelineProps;
    timelineProps.StartTime(std::chrono::seconds(0));
    timelineProps.MinSeekTime(std::chrono::seconds(0));
    timelineProps.Position(std::chrono::seconds(s_currentPlayPos));
    timelineProps.MaxSeekTime(std::chrono::seconds(s_songList[s_currentSongIdx].lengthSecs));
    timelineProps.EndTime(std::chrono::seconds(s_songList[s_currentSongIdx].lengthSecs));
    smtc.UpdateTimelineProperties(timelineProps);
    printf("Updated media timeline\n");
}

void play(const SystemMediaTransportControls &smtc) {
    if (s_isPlaying) {
        return;
    }
    s_isPlaying = true;
    const LARGE_INTEGER timeout{ .QuadPart = -10'000'000LL };
    if (!SetWaitableTimer(s_mockPlayerTimer, &timeout, 1000, nullptr, nullptr, FALSE)) {
        printf("Failed to start mock player timer\n");
        exit(1);
    }
    updatePlaybackStatus(smtc);
    printf("Playing\n");
}

void pause(const SystemMediaTransportControls &smtc) {
    if (!s_isPlaying) {
        return;
    }
    s_isPlaying = false;
    if (!CancelWaitableTimer(s_mockPlayerTimer)) {
        printf("Failed to stop mock player timer\n");
        exit(1);
    }
    updatePlaybackStatus(smtc);
    updateTimeline(smtc);
    printf("Paused\n");
}

void changeTrack(const SystemMediaTransportControls &smtc, bool isNext) {
    pause(smtc);
    if (isNext) {
        s_currentSongIdx++;
        if (s_currentSongIdx >= s_songList.size()) {
            s_currentSongIdx = 0;
        }
    } else {
        s_currentSongIdx--;
        if (s_currentSongIdx < 0) {
            s_currentSongIdx = s_songList.size() - 1;
        }
    }
    s_currentPlayPos = 0;
    updateSongInfo(smtc);
    play(smtc);
    updateTimeline(smtc);
}

void seek(const SystemMediaTransportControls &smtc, int seekPos) {
    if (seekPos < 0) {
        seekPos = 0;
    } else if (seekPos > s_songList[s_currentSongIdx].lengthSecs) {
        changeTrack(smtc, true);
        return;
    }
    pause(smtc);
    printf("Seeking to %d\n", seekPos);
    s_currentPlayPos = seekPos;
    updateTimeline(smtc);
    play(smtc);
}

void tick(const SystemMediaTransportControls &smtc) {
    s_currentPlayPos += 1;
    if (s_currentPlayPos > s_songList[s_currentSongIdx].lengthSecs) {
        s_currentSongIdx++;
        if (s_currentSongIdx >= s_songList.size()) {
            s_currentSongIdx = 0;
        }
        s_currentPlayPos = 0;
        updateSongInfo(smtc);
    }
    if (s_currentPlayPos % 5 == 0) {
        updateTimeline(smtc);
    }
    printf("Current position: %d\n", s_currentPlayPos);
}

void test() {
    HANDLE handleStdin = GetStdHandle(STD_INPUT_HANDLE);
    if (handleStdin == INVALID_HANDLE_VALUE) {
        printf("Not a console!\n");
        return;
    }

    s_queueEvent = CreateEventW(nullptr, false, false, nullptr);
    s_mockPlayerTimer = CreateWaitableTimerW(nullptr, false, nullptr);

    try {
        s_mediaPlayer = MediaPlayer();
        auto smtc = s_mediaPlayer.SystemMediaTransportControls();
        s_mediaPlayer.CommandManager().IsEnabled(false);

        smtc.IsPlayEnabled(true);
        smtc.IsPauseEnabled(true);
        smtc.IsPreviousEnabled(true);
        smtc.IsNextEnabled(true);

        smtc.ButtonPressed([](const SystemMediaTransportControls &sender, const SystemMediaTransportControlsButtonPressedEventArgs &args) {
            std::lock_guard<std::mutex> guard(s_queueMutex);
            s_queue.emplace(button_command{.m_button = args.Button()});
            SetEvent(s_queueEvent);
        });
        smtc.PlaybackPositionChangeRequested([](const SystemMediaTransportControls &sender, const winrt::Windows::Media::PlaybackPositionChangeRequestedEventArgs &args) {
            int seekPos = std::chrono::duration_cast<std::chrono::seconds>(args.RequestedPlaybackPosition()).count();
            std::lock_guard<std::mutex> guard(s_queueMutex);
            s_queue.emplace(seek_command{.m_seekPos = seekPos});
            SetEvent(s_queueEvent);
        });

        smtc.IsEnabled(true);
        smtc.AutoRepeatMode(MediaPlaybackAutoRepeatMode::List);

        updateSongInfo(smtc);
        updatePlaybackStatus(smtc);
        updateTimeline(smtc);

        printf("We are playing media!\n");
        
        HANDLE waitHandles[] { handleStdin, s_queueEvent, s_mockPlayerTimer };
        do {
            switch (WaitForMultipleObjects(ARRAYSIZE(waitHandles), waitHandles, FALSE, INFINITE)) {
            case WAIT_OBJECT_0: {
                INPUT_RECORD inputRecord;
                DWORD readCount;
                if (!ReadConsoleInputW(handleStdin, &inputRecord, 1, &readCount)) {
                    printf("Error reading console input!\n");
                    goto loop_exit;
                }
                switch (inputRecord.EventType) {
                case KEY_EVENT: {
                    auto keyEv = inputRecord.Event.KeyEvent;
                    if (!keyEv.bKeyDown) {
                        break;
                    }
                    switch (keyEv.wVirtualKeyCode) {
                    case VK_SPACE: {
                        if (s_isPlaying) {
                            pause(smtc);
                        } else {
                            play(smtc);
                        }
                    } break;
                    case 'K':
                    case VK_UP: {
                        changeTrack(smtc, false);
                    } break;
                    case 'J':
                    case VK_DOWN: {
                        changeTrack(smtc, true);
                    } break;
                    default:
                        break;
                    }
                } break;
                default:
                    break;
                }
            } break;
            case WAIT_OBJECT_0 + 1: {
                std::lock_guard<std::mutex> guard(s_queueMutex);
                while (!s_queue.empty()) {
                    const auto &cmd = s_queue.front();
                    if (const auto *btn = std::get_if<button_command>(&cmd)) {
                        printf("Button: %d\n", btn->m_button);
                        switch (btn->m_button) {
                        case SystemMediaTransportControlsButton::Play:
                            play(smtc);
                            break;
                        case SystemMediaTransportControlsButton::Pause:
                            pause(smtc);
                            break;
                        case SystemMediaTransportControlsButton::Next:
                            changeTrack(smtc, true);
                            break;
                        case SystemMediaTransportControlsButton::Previous:
                            changeTrack(smtc, false);
                            break;
                        default:
                            break;
                        }
                    } else if (const auto *seek = std::get_if<seek_command>(&cmd)) {
                        ::seek(smtc, seek->m_seekPos);
                    }
                    s_queue.pop();
                }
            } break;
            case WAIT_OBJECT_0 + 2: {
                tick(smtc);
            } break;
            case WAIT_FAILED:
            default:
                printf("Error in WaitForMultipleObjects.\n");
                goto loop_exit;
            }
        } while (true);
    loop_exit:;
    } catch (const winrt::hresult_error &ex) {
        wprintf(L"Got HRESULT %x - %s\n", ex.code().value, ex.message().c_str());
    }
}

int wmain(int argc, wchar_t *argv[]) {
    winrt::init_apartment();
    test();
    return 0;
}

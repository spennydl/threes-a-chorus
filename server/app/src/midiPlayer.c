#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "hal/timeutils.h"
#include "lib/midi-parser.h"
#include "midiPlayer.h"
#include "tcp.h"

struct MidiEventNode
{
    struct MidiEventNode* next;
    int channel;
    unsigned int status;
    uint8_t param1;
    uint8_t param2;
    int64_t vtime;
    int64_t currentVTime;
};

static atomic_bool readyToPlay = false;

static bool running = true;

static int ppq;
static int bpm = 90;

static pthread_t playerThread;

static struct MidiEventNode* channelHeads[16] = { 0 };
static struct MidiEventNode* currentEventNodes[16] = { 0 };
static int instruments[16] = { 0 };

// I really don't wanna malloc this array
static int listeners[16][3] = { { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 },
                                { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 },
                                { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 },
                                { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 },
                                { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 },
                                { -1, -1, -1 } };

static int serverInstance = 42;

static long long
vTimeInNs(long long vtime)
{
    return (long long)(vtime * (60000000000 / (bpm * ppq)));
}

static void
onMessageRecieved(void* instance, const char* newMessage, int socketFd)
{
    (void)instance;

    char buffer[MAX_LEN] = { 0 };
    strncpy(buffer, newMessage, MAX_LEN - 1);
    char* command = strtok(buffer, " ");
    (void)(command);
    int channel = atoi(strtok(NULL, " "));

    printf("Registering new socket to send to for channel %d\n", channel);

    // Bad channel - first try to join any channel with no listeners for
    // diversity reasons
    if (channelHeads[channel] == NULL) {
        for (int i = 0; i < 16; i++) {
            // I know the long && is weird but it avoids another for loop
            if (channelHeads[i] != NULL &&
                (listeners[i][0] == -1 && listeners[i][1] == -1 &&
                 listeners[i][2] == -1)) {
                printf(
                  "Tried to join invalid channel %d. Had to fallback to %d\n",
                  channel,
                  i);
                channel = i;
                break;
            }
        }
    }

    // Bad channel - now just try to join the first available channel
    if (channelHeads[channel] == NULL) {
        for (int i = 0; i < 16; i++) {
            if (channelHeads[i] != NULL) {
                printf(
                  "Tried to join invalid channel %d. Had to fallback to %d\n",
                  channel,
                  i);
                channel = i;
                break;
            }
        }
    }

    for (int j = 0; j < 3; j++) {
        if (listeners[channel][j] == -1) {

            char buffer[MAX_LEN] = { 0 };
            snprintf(buffer,
                     MAX_LEN - 1,
                     "%d;%d",
                     MIDI_STATUS_PGM_CHANGE,
                     instruments[channel]);
            Tcp_sendTcpServerResponse(buffer, socketFd);

            listeners[channel][j] = socketFd;
            return;
        }
    }

    printf(
      "Somehow there was no room to register. This shouldn't be possible.\n");
}

static void
clearNode(struct MidiEventNode* node,
          struct MidiEventNode* head,
          bool firstTime)
{
    if (node == NULL || (!firstTime && node == head)) {
        return;
    }

    clearNode(node->next, head, false);
    free(node);
}

static void
clearAllEventNodes()
{
    for (int i = 0; i < 16; i++) {
        clearNode(channelHeads[i], channelHeads[i], true);
        channelHeads[i] = NULL;
    }
}

static void
parse_and_dump(struct midi_parser* parser)
{
    clearAllEventNodes();
    struct MidiEventNode* prev[16] = { 0 };
    enum midi_parser_status status;
    int channel = -1;
    while (1) {
        status = midi_parse(parser);
        switch (status) {

            case MIDI_PARSER_EOB:
                return;

            case MIDI_PARSER_ERROR:
                printf("Error during midi parsing");
                return;

            case MIDI_PARSER_HEADER:
                printf("header\n");
                printf("  size: %d\n", parser->header.size);
                printf("  format: %d [%s]\n",
                       parser->header.format,
                       midi_file_format_name(parser->header.format));
                printf("  tracks count: %d\n", parser->header.tracks_count);
                printf("  time division: %d\n", parser->header.time_division);
                ppq = parser->header.time_division;
                break;

            case MIDI_PARSER_TRACK_MIDI:

                channel = parser->midi.channel;
                struct MidiEventNode* eventNode =
                  malloc(sizeof(struct MidiEventNode));

                eventNode->status = parser->midi.status;
                eventNode->vtime = parser->vtime;
                eventNode->currentVTime = parser->vtime;
                eventNode->param1 = parser->midi.param1;
                eventNode->param2 = parser->midi.param2;
                eventNode->channel = parser->midi.channel;
                eventNode->next = NULL;

                if (channelHeads[channel] == NULL) {
                    channelHeads[channel] = eventNode;
                } else {
                    prev[channel]->next = eventNode;
                }

                prev[channel] = eventNode;
                break;

            case MIDI_PARSER_TRACK:
                break;

            case MIDI_PARSER_TRACK_SYSEX:
                break;

            case MIDI_PARSER_TRACK_META:
                break;

            default:
                printf("unhandled midi state: %d\n", status);
                return;
        }
    }
}

static int
parseMidiFile(const char* path)
{
    struct stat st;

    if (access(path, F_OK) != 0) {
        fprintf(stderr, "MIDI file not found\n");
        return 1;
    }

    if (stat(path, &st)) {
        fprintf(stderr, "stat(%s):\n", path);
        return 1;
    }

    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "err %d open(%s):\n", fd, path);
        return 1;
    }

    void* mem = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if (mem == MAP_FAILED) {
        fprintf(stderr, "mmap fail(%s):\n", path);
        close(fd);
        return 1;
    }

    struct midi_parser parser;
    parser.state = MIDI_PARSER_INIT;
    parser.size = st.st_size;
    parser.in = mem;

    parse_and_dump(&parser);

    munmap(mem, st.st_size);
    close(fd);
    return 0;
}

Server_Status
MidiPlayer_initialize()
{
    for (int i = 0; i < 16; i++) {
        for (int j = 0; j < 3; j++) {
            listeners[i][j] = -1;
        }
    }

    TcpObserver exampleObserver;
    exampleObserver.instance = &serverInstance;
    exampleObserver.notification = onMessageRecieved;

    Tcp_attachToTcpServer(&exampleObserver);

    if (pthread_create(&playerThread, NULL, &midiPlayerWorker, NULL) != 0) {
        fprintf(stderr,
                "Error creating thread during MIDI player initialization!");
        return SERVER_ERROR;
    }

    return SERVER_OK;
}

void
MidiPlayer_cleanup()
{
    running = false;
    pthread_join(playerThread, NULL);
}

Server_Status
MidiPlayer_playMidiFile(char* path)
{
    readyToPlay = false;

    clearAllEventNodes();
    int res = parseMidiFile(path);

    if (res == 1) {
        return SERVER_ERROR;
    }

    for (int i = 0; i < 16; i++) {
        currentEventNodes[i] = channelHeads[i];
    }

    readyToPlay = true;

    return SERVER_OK;
}

void
MidiPlayer_setBpm(int newBpm)
{
    bpm = newBpm;
}

void
MidiPlayer_getRandomMidiPath(char* buffer)
{
    int numberOfMidis = 0;
    char* midiFileNames[128] = { 0 };

    DIR* d = opendir("midis");
    struct dirent* dir;

    while ((dir = readdir(d)) != NULL) {
        if (dir->d_type != DT_DIR) {
            midiFileNames[numberOfMidis++] = dir->d_name;
        }
    }

    int index = (rand() % numberOfMidis);
    snprintf(buffer, 256, "midis/%s", midiFileNames[index]);
}

void*
midiPlayerWorker(void* p)
{
    (void)p;
    bool anyTracksPlaying = true;

    while (running) {
        if (!readyToPlay) {
            continue;
        }

        long long shortestVTime = LLONG_MAX;

        for (int i = 0; i < 16; i++) {
            if (channelHeads[i] != NULL && currentEventNodes[i] != NULL) {
                shortestVTime =
                  currentEventNodes[i]->currentVTime < shortestVTime
                    ? currentEventNodes[i]->currentVTime
                    : shortestVTime;
            }
        }

        Timeutils_sleepForNs(vTimeInNs(shortestVTime));

        anyTracksPlaying = false;
        for (int i = 0; i < 16; i++) {
            if (channelHeads[i] != NULL && currentEventNodes[i] != NULL) {
                anyTracksPlaying = true;
                if (currentEventNodes[i]->currentVTime == shortestVTime) {
                    currentEventNodes[i]->currentVTime =
                      currentEventNodes[i]->vtime;

                    if (currentEventNodes[i]->status ==
                        MIDI_STATUS_PGM_CHANGE) {
                        instruments[i] = currentEventNodes[i]->param1;
                    }

                    for (int j = 0; j < 3; j++) {
                        char buffer[MAX_LEN] = { 0 };
                        snprintf(buffer,
                                 MAX_LEN - 1,
                                 "%d;%d",
                                 currentEventNodes[i]->status,
                                 currentEventNodes[i]->param1);
                        if (listeners[i][j] != -1) {
                            int res = Tcp_sendTcpServerResponse(
                              buffer, listeners[i][j]);
                            if (res == -1) {
                                printf("Channel %d #%d disconnected\n", i, j);
                                listeners[i][j] = -1;
                            }
                        }
                    }

                    currentEventNodes[i] = currentEventNodes[i]->next;
                } else {
                    currentEventNodes[i]->currentVTime -= shortestVTime;
                }
            }
        }

        if (!anyTracksPlaying) {
            for (int i = 0; i < 16; i++) {
                currentEventNodes[i] = channelHeads[i];
            }
        }
    }

    return NULL;
}
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <stdatomic.h>
#include <pthread.h>
#include <stdlib.h>
#include <limits.h>

#include "hal/timeutils.h"
#include "midiPlayer.h"
#include "lib/midi-parser.h"
#include "tcp.h"

struct MidiEventNode {
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

static struct MidiEventNode* channelHeads[16] = {0};
static struct MidiEventNode* currentEventNodes[16] = {0};
static int instruments[16] = {0};

// I really don't wanna malloc this array
static int listeners[16][3] = {
  {-1,-1,-1},
  {-1,-1,-1},
  {-1,-1,-1},
  {-1,-1,-1},
  {-1,-1,-1},
  {-1,-1,-1},
  {-1,-1,-1},
  {-1,-1,-1},
  {-1,-1,-1},
  {-1,-1,-1},
  {-1,-1,-1},
  {-1,-1,-1},
  {-1,-1,-1},
  {-1,-1,-1},
  {-1,-1,-1},
  {-1,-1,-1}
};

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

    char buffer[MAX_LEN] = {0};
    strncpy(buffer, newMessage, MAX_LEN - 1);
    char* command = strtok(buffer, " ");
    (void)(command);
    int channel = atoi(strtok(NULL, " "));

    printf("Registering new socket to send to for channel %d\n", channel);
    
    for(int j = 0; j < 3; j++) {
      if(listeners[channel][j] == -1) {

        char buffer[MAX_LEN] = {0};
        snprintf(buffer, MAX_LEN - 1, "%d;%d", MIDI_STATUS_PGM_CHANGE, instruments[channel]);
        Tcp_sendTcpServerResponse(buffer, socketFd);

        listeners[channel][j] = socketFd;
        return;
      }
    }
    
    printf("Somehow there was no room to register. This shouldn't be possible.\n");
}


static void
clearNode(struct MidiEventNode* node, struct MidiEventNode* head, bool firstTime)
{
    if(node == NULL || (!firstTime && node == head)) {
        return;
    }

    clearNode(node->next, head, false);
    free(node);
}

static void
clearAllEventNodes()
{
    for(int i = 0; i < 16; i++) {
      clearNode(channelHeads[i], channelHeads[i], true);
      channelHeads[i] = NULL;
    }
}

static void makeLinkedListLoop(struct MidiEventNode* currentMidiHead)
{
    if(currentMidiHead == NULL) {
      //perror("Midi head is null when trying to make loop\n");
      return;
    }
    else {
      printf("Making channel %d loop because it is valid\n", currentMidiHead->channel);
    }

    struct MidiEventNode* current = currentMidiHead;
    while(current->next != NULL) {
      current = current->next;
    }
    current->next = currentMidiHead;
}

static void parse_and_dump(struct midi_parser *parser, long long* trackLengths)
{
  clearAllEventNodes();
  struct MidiEventNode* prev[16] = {0};
  enum midi_parser_status status;

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
        printf("  format: %d [%s]\n", parser->header.format, midi_file_format_name(parser->header.format));
        printf("  tracks count: %d\n", parser->header.tracks_count);
        printf("  time division: %d\n", parser->header.time_division);
        ppq = parser->header.time_division;
        break;

      case MIDI_PARSER_TRACK_MIDI:

        // Keep track of longest track for adding a buffer at the end
        trackLengths[parser->midi.channel] += parser->vtime;

        struct MidiEventNode* eventNode = malloc(sizeof(struct MidiEventNode));
        int channel = parser->midi.channel;
        eventNode->status = parser->midi.status;
        eventNode->vtime = parser->vtime;
        eventNode->currentVTime = parser->vtime;
        eventNode->param1 = parser->midi.param1;
        eventNode->param2 = parser->midi.param2;
        eventNode->channel = parser->midi.channel;
        eventNode->next = NULL;

        if(channelHeads[channel] == NULL) {
          channelHeads[channel] = eventNode;
        }
        else {
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
parseMidiFile(const char *path)
{
  struct stat st;

  if(access(path, F_OK) != 0) {
    printf("MIDI file not found\n");
    return 1;
  }

  if (stat(path, &st)) {
    printf("stat(%s):\n", path);
    return 1;
  }

  int fd = open(path, O_RDONLY);
  if (fd < 0) {
    printf("open(%s):\n", path);
    return 1;
  }

  void *mem = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
  if (mem == MAP_FAILED) {
    printf("mmap(%s):\n", path);
    close(fd);
    return 1;
  }

  struct midi_parser parser;
  parser.state = MIDI_PARSER_INIT;
  parser.size  = st.st_size;
  parser.in    = mem;

  long long trackLengths[16] = {0};

  parse_and_dump(&parser, trackLengths);

  long long longestTrackLength = 0;

  for(int i = 0; i < 16; i++) {
    longestTrackLength = trackLengths[i] > longestTrackLength ? trackLengths[i] : longestTrackLength;
  }

  for(int i = 0; i < 16; i++) {

    if(channelHeads[i] == NULL) {
      continue;
    }

    long long padding = longestTrackLength - trackLengths[i];
    struct MidiEventNode* current = channelHeads[i];

    // Create padding node and add to the end of the midi nodes
    struct MidiEventNode* paddingNode = malloc(sizeof(struct MidiEventNode));
    paddingNode->status = MIDI_STATUS_NOTE_OFF;
    paddingNode->vtime = padding;
    paddingNode->param1 = 1;
    paddingNode->param2 = 1;
    paddingNode->next = NULL;
    paddingNode->channel = i;

    while(current->next != NULL) {
      current = current->next;
    }

    current->next = paddingNode;
  }


  munmap(mem, st.st_size);
  close(fd);
  return 0;
}

void
MidiPlayer_initialize()
{
    for(int i = 0; i < 16; i++) {
      for(int j = 0; j < 3; j++) {
        listeners[i][j] = -1;
      }
    }

    TcpObserver exampleObserver;
    exampleObserver.instance = &serverInstance;
    exampleObserver.notification = onMessageRecieved;

    Tcp_attachToTcpServer(&exampleObserver);

    pthread_create(&playerThread, NULL, &midiPlayerWorker, NULL);
}

void
MidiPlayer_cleanup()
{
    running = false;
    pthread_join(playerThread, NULL);
}

void
MidiPlayer_playMidiFile(char* path)
{
    readyToPlay = false;

    clearAllEventNodes();
    int res = parseMidiFile(path);

    if(res == 1) {
      return;
    }

    for(int i = 0; i < 16; i++) {
      makeLinkedListLoop(channelHeads[i]);
      currentEventNodes[i] = channelHeads[i];
    }
    
    readyToPlay = true;
}

void
MidiPlayer_setBpm(int newBpm)
{
  bpm = newBpm;
}

void*
midiPlayerWorker(void* p)
{
  (void)p;

  while(running) {
    if(!readyToPlay) {
      continue;
    }

    long long shortestVTime = LLONG_MAX;

    for(int i = 0; i < 16; i++) {
      if(channelHeads[i] != NULL) {
        shortestVTime = currentEventNodes[i]->currentVTime < shortestVTime ? currentEventNodes[i]->currentVTime : shortestVTime;
      }
    }

    Timeutils_sleepForNs(vTimeInNs(shortestVTime));

    for(int i = 0; i < 16; i++) {
      if(channelHeads[i] != NULL) {
        if(currentEventNodes[i]->currentVTime == shortestVTime) {
          currentEventNodes[i]->currentVTime = currentEventNodes[i]->vtime;
          
          if(currentEventNodes[i]->status == MIDI_STATUS_PGM_CHANGE) {
            printf("Instrument for channel %d is now %d\n", i, currentEventNodes[i]->param1);
            instruments[i] = currentEventNodes[i]->param1;
          }

          for(int j = 0; j < 3; j++) {
            char buffer[MAX_LEN] = {0};
            snprintf(buffer, MAX_LEN - 1, "%d;%d", currentEventNodes[i]->status, currentEventNodes[i]->param1);
            if(listeners[i][j] != -1) {
              int res = Tcp_sendTcpServerResponse(buffer, listeners[i][j]);
              if(res == -1) {
                printf("Channel %d #%d disconnected\n", i, j);
                listeners[i][j] = -1;
              }
            }
          }
          
          currentEventNodes[i] = currentEventNodes[i]->next;
        }
        else {
          currentEventNodes[i]->currentVTime -= shortestVTime;
        }
      }
    }
  }

  return NULL;
}
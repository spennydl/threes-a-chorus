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

#include "hal/timeutils.h"
#include "midiPlayer.h"
#include "lib/midi-parser.h"
#include "das/fmplayer.h"

struct MidiEventNode {
    struct MidiEventNode* next;
    int channel;
    unsigned int status;
    uint8_t param1;
    uint8_t param2;
    int64_t vtime;
};

static atomic_bool readyToPlay = false;

static bool running = true;

static struct MidiEventNode* currentMidiHead;
static struct MidiEventNode* currentNode;
static int channelToPlayWith = -1;
static int ppq;
static int bpm = 105;
static long long nsLeft = 0;
static atomic_llong timeUntilNextEvent = 0;
static long long beatOffset = 0;

static pthread_t playerThread;

static long long
vTimeInNs(int vtime)
{
  return (long long)(vtime * (60000000000 / (bpm * ppq)));
}

static long long
nsPerBeat()
{
  return 60000000000 / bpm;
}

static void
clearNode(struct MidiEventNode* node)
{
    if(node == NULL) {
        return;
    }

    clearNode(node->next);
    free(node);
}

static void
clearAllEventNodes()
{
    clearNode(currentMidiHead);
}

static void makeLinkedListLoop()
{
    if(currentMidiHead == NULL) {
      perror("Midi head is null when trying to make loop\n");
      return;
    }

    struct MidiEventNode* current = currentMidiHead;
    while(current->next != NULL) {
      current = current->next;
    }
    current->next = currentMidiHead;
}

static void parse_and_dump(struct midi_parser *parser, int channel, long long* trackLengths)
{
  clearAllEventNodes();
  currentMidiHead = NULL;
  struct MidiEventNode* prev;
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

        // TODO: Make it so if the channel length is 0 then  we don't play with it
        // Probably later on and not in this function
        channelToPlayWith = channel % parser->header.tracks_count;
        ppq = parser->header.time_division;
        break;

      case MIDI_PARSER_TRACK_MIDI:

        // Keep track of longest track for adding a buffer at the end
        trackLengths[parser->midi.channel] += parser->vtime;

        if(parser->midi.channel != channelToPlayWith) {
          break;
        }

        struct MidiEventNode* eventNode = malloc(sizeof(struct MidiEventNode));
        eventNode->status = parser->midi.status;
        eventNode->vtime = parser->vtime;
        eventNode->param1 = parser->midi.param1;
        eventNode->param2 = parser->midi.param2;
        eventNode->channel = parser->midi.channel;
        eventNode->next = NULL;

        if(currentMidiHead == NULL) {
          currentMidiHead = eventNode;
        }
        else {
          prev->next = eventNode;
        }

        prev = eventNode;
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
parseMidiFileChannel(const char *path, int channel)
{
  struct stat st;

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

  parse_and_dump(&parser, channel, trackLengths);

  long long longestTrackLength = 0;

  for(int i = 0; i < 16; i++) {
    longestTrackLength = trackLengths[i] > longestTrackLength ? trackLengths[i] : longestTrackLength;
  }

  long long padding = longestTrackLength - trackLengths[channelToPlayWith];
  struct MidiEventNode* current = currentMidiHead;

  // Create padding node and add to the end of the midi nodes
  struct MidiEventNode* paddingNode = malloc(sizeof(struct MidiEventNode));
  paddingNode->status = MIDI_STATUS_NOTE_OFF;
  paddingNode->vtime = padding;
  paddingNode->param1 = 0;
  paddingNode->param2 = 0;
  paddingNode->next = NULL;
  paddingNode->channel = channelToPlayWith;

  while(current->next != NULL) {
    current = current->next;
  }

  current->next = paddingNode;

  munmap(mem, st.st_size);
  close(fd);
  return 0;
}

void
MidiPlayer_initialize()
{
    pthread_create(&playerThread, NULL, &midiPlayerWorker, NULL);
}

void
MidiPlayer_cleanup()
{
    running = false;
    pthread_join(playerThread, NULL);
}

void
MidiPlayer_playMidiFile(char* path, int channelNumber)
{
    readyToPlay = false;
    parseMidiFileChannel(path, channelNumber);

    timeUntilNextEvent = 0;
    currentNode = currentMidiHead;
    makeLinkedListLoop();

    struct MidiEventNode* current = currentMidiHead;
    long long beatOffsetInNs = nsPerBeat() * beatOffset;

    printf("Beat offset %lld or %lld in ns\n", beatOffset, beatOffsetInNs);

    while(beatOffsetInNs > nsPerBeat()) {
      long long delay = vTimeInNs(current->vtime);
      beatOffsetInNs -= delay;
      current = current->next;
    }

    Timeutils_sleepForNs(beatOffsetInNs);

    currentNode = current;

    readyToPlay = true;
}

void
MidiPlayer_playNextBeat()
{
    nsLeft += nsPerBeat();
}

void
MidiPlayer_setBeatOffset(long long newBeatOffset)
{
  printf("New beat offset %lld\n", newBeatOffset);
  beatOffset = newBeatOffset;
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
    timeUntilNextEvent = 0;

    while(running) {
        if(!readyToPlay || currentNode == NULL || nsLeft <= 0) {
            continue;
        }

        if(nsLeft < 0) {
            nsLeft = 0;
        }

        long long chunkOfTime = (long long)((60000000000 / (bpm * ppq)));

        //printf("%ld - %lld\n", timeUntilNextEvent, chunkOfTime);

        if(timeUntilNextEvent > 0) { 
            timeUntilNextEvent -= chunkOfTime;
            nsLeft -= chunkOfTime;
            Timeutils_sleepForNs(chunkOfTime);
        }
        else {
            if(currentNode->status == MIDI_STATUS_NOTE_ON) {
                // C2 = 0 for fm
                // C2 = 36 for other
                // So the note we want to  play is - 36 away

                int fmNote = currentNode->param1 - 36;

                FmPlayer_setNote(fmNote);

                FmPlayer_controlNote(NOTE_CTRL_NOTE_ON);
                
            }
            else if(currentNode->status == MIDI_STATUS_NOTE_OFF) {
                FmPlayer_controlNote(NOTE_CTRL_NOTE_OFF);
            }
            

            currentNode = currentNode->next;
            timeUntilNextEvent = vTimeInNs(currentNode->vtime);
        }
        
    }

    return NULL;
}
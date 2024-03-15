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
static int bpm = 120;
static long long msLeft = 0;
static atomic_long timeUntilNextEvent = 0;

static pthread_t playerThread;

static void clearNode(struct MidiEventNode* node)
{
    if(node == NULL) {
        return;
    }

    clearNode(node->next);
    free(node);
}

static void clearAllEventNodes()
{
    clearNode(currentMidiHead);
}

static void parse_and_dump(struct midi_parser *parser, int channel)
{
  clearAllEventNodes();
  currentMidiHead = NULL;
  struct MidiEventNode* prev;
  enum midi_parser_status status;

  while (1) {
    status = midi_parse(parser);
    switch (status) {
    case MIDI_PARSER_EOB:
      puts("eob");
      return;

    case MIDI_PARSER_ERROR:
      puts("error");
      return;

    case MIDI_PARSER_INIT:
      //puts("init");
      break;

    case MIDI_PARSER_HEADER:
      printf("header\n");
      printf("  size: %d\n", parser->header.size);
      printf("  format: %d [%s]\n", parser->header.format, midi_file_format_name(parser->header.format));
      printf("  tracks count: %d\n", parser->header.tracks_count);
      printf("  time division: %d\n", parser->header.time_division);
      channelToPlayWith = channel % parser->header.tracks_count;
      ppq = parser->header.time_division;
      break;

    case MIDI_PARSER_TRACK:
      //puts("track");
      //printf("  length: %d\n", parser->track.size);
      break;

    case MIDI_PARSER_TRACK_MIDI:
      if(parser->midi.channel != channelToPlayWith) {
        break;
      }
      struct MidiEventNode* eventNode = malloc(sizeof(struct MidiEventNode));
      eventNode->status = parser->midi.status;
      eventNode->vtime = parser->vtime;
      eventNode->param1 = parser->midi.param1;
      eventNode->param2 = parser->midi.param2;
      eventNode->next = NULL;
      eventNode->channel = parser->midi.channel;

      if(currentMidiHead == NULL)
      {
        currentMidiHead = eventNode;
      }
      else
      {
        prev->next = eventNode;
      }

      prev = eventNode;

      break;

    case MIDI_PARSER_TRACK_META:
      break;
      printf("track-meta\n");
      printf("  time: %lld\n", parser->vtime);
      printf("  type: %d [%s]\n", parser->meta.type, midi_meta_name(parser->meta.type));
      printf("  length: %d\n", parser->meta.length);
      break;

    case MIDI_PARSER_TRACK_SYSEX:
      break;
      puts("track-sysex");
      printf("  time: %lld\n", parser->vtime);
      break;

    default:
      printf("unhandled state: %d\n", status);
      return;
    }
  }
}

static int parse_file(const char *path, int channel)
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

  parse_and_dump(&parser, channel);

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
    parse_file(path, channelNumber);
    timeUntilNextEvent = 0;
    currentNode = currentMidiHead;
    readyToPlay = true;
}

void
MidiPlayer_canPlayNextBeat()
{
    msLeft += 60000000000 / bpm;
}

void*
midiPlayerWorker(void* p)
{
    (void)p;
    timeUntilNextEvent = 0;

    while(running)
    {
        if(!readyToPlay || currentNode == NULL || msLeft <= 0) {
            continue;
        }

        if(msLeft < 0) {
            msLeft = 0;
        }

        long long chunkOfTime = (long long)((1000000 * 60000.0 / (bpm * ppq)));

        //printf("%ld - %lld\n", timeUntilNextEvent, chunkOfTime);

        if(timeUntilNextEvent > 0) { 
            timeUntilNextEvent -= chunkOfTime;
            msLeft -= chunkOfTime;
            Timeutils_sleepForNs(chunkOfTime);
        }
        else {
            if(currentNode->status == MIDI_STATUS_NOTE_ON)
            {
                // C2 = 0 for fm
                // C2 = 36 for other
                // So the note we want to  play is - 36 away

                int fmNote = currentNode->param1 - 36;

                FmPlayer_setNote(fmNote);

                FmPlayer_controlNote(NOTE_CTRL_NOTE_ON);
                
            }
            else if(currentNode->status == MIDI_STATUS_NOTE_OFF)
            {
                FmPlayer_controlNote(NOTE_CTRL_NOTE_OFF);
            }
            

            currentNode = currentNode->next;
            timeUntilNextEvent = (long long)(currentNode->vtime * (1000000 * 60000.0 / (bpm * ppq)));
        }
        
    }

    return NULL;
}
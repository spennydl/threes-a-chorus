# Three's a Chorus

Three’s a Chorus is an interactive demo created by Louie Lu (louie_lu@sfu.ca),
Jet Simon (jets@sfu.ca), and Spencer Leslie (sla489@sfu.ca). It was originally
built as a group project for an Embedded Systems class at Simon Fraser
University.

The demo features three "singers", each of which have their own personality and
generate audio according to their mood. Their mood will change when they are
interacted with through the sensor channels each one has: a button, a
potentiometer, an accelerometer, a light sensor, and a proximity sensor. Each
singer can also detect when it is placed in a designated spot on the demo surface
by reading RFID tags embedded into small objects. When placed on one such spot,
the singer will begin to stream and play MIDI events from a control server. Each
singer placed on a spot will sync up with the others enabling them to sing together.

### Codemap

The project comprises the following libraries:

- `client/app` :: Main client application logic. Contains sensory integration
  and mood calculation.
- `client/common` :: Common support library.
- `client/das` :: Digital Audio Synthesis library. Contains the FM synthesizer
  and an FmPlayer that drives the synthesizer and outputs sound.
- `client/hal` :: Provides abstraction over hardware access (gpio, i2c, etc) and
  modules for interacting with some sensors.
- `client/bin` :: Binary libraries required by the client.
- `server/app` :: Main server application logic.
- `server/hal` :: Provides abstraction over hardware and networking.
- `server/lib` :: Contains 3rd party libraries used by the server for midi
  parsing. 

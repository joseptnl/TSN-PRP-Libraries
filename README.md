# TSN-Libraries

The library can be used to send a batch of k Ethernet frames. All the frames of the same batch belong to the same Ethernet standard. The standars can be:
* Parallel Redundancy Protocol (PRP)
* Time Sensitive Networking (TSN): The frames can be sent using an R-tag that is identified by the FRER mechanism to add FT capabilities.

Additionally, the library provides a logging feature to listen a particular NIC and create a logging file with all the enregistered activity.

## Use

First you should make sure the directory is cleared of compiled files. Then, you can use the *make* command with the parameters listed bellow:
* prpstest: Send prp frames
* tsnstest: Send tsn frames
* rtest: Receive frames.

## Structure

The library has a Layers Architecture. The internal layers are:
* generics: Hiddes direct interaction with Linux OS functions using sockets.
* packetio: Avoids the need to know the socket concept, allowing the developer to work with NICs directly.
* ethframes: Provides function utils to build the Ethernet frames.

Above the internal layers there are the application layers:
* tsn: Hiddes logic of sending a copy of each frame through k NICs
* prp: Hiddes logic of sending a copy of each frame through 2 NICs
* log: Listen a particular NIC and stores representative data from each received frame in a file named as the NIC's name.

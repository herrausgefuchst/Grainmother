/// MULTIEFFECT Version 0.01 
/// 
/// 2023/10/22
///
/// CPU ~ 9...15 %
///
/// connected the In-Out-Structure with BELA
/// AudioPlayer and Oscillator works, 
/// Menu works, saving and loading presets works
/// saving = 1 mode switch 
/// LEDS in GUI are working
/// Tempotapping works 
/// bypassing works 
/// FX Edit Focus works: potentiometer switch to their corresponding effect Parameter (of course this also wont work fpr FX3)
/// GUI Display works, although could be prettier... (TODO: check how to insert empty spaces in strings)
/// Scope can be activated via flag in PREDEF.h
///
/// TODO: didn't check the OSC Output yet
/// TODO: write OSC messages in O2O.main 
/// TODO: check for analog ins (buttons, potentiometers)
/// TODO: GUI needs some program starts and stops and window refreshments till it works, dont know why...
/// TODO: FX3 Button doesnt work, because there is no effect referenced to it yet
/// TODO: in general: the effects only process their parameter ramps, but don't make anything wirth sound yet....
/// TODO: didnt check the Analog output LEDS yet, should work with uncommenting analogWrite in updateLEDS()
/// TODO: didnt check if the values save correctly in json file, if yes, mode switch is not dangerous
/// TODO: Analog In not checked yet 
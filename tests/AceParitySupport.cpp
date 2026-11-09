// TSRE GenX regression support. GNU GPL v3 or later. See ../LICENSE.md.
// Link the real AceLib.cpp/ReadFile.cpp without renderer/cache/application code.
// These definitions supply only storage and raw-buffer ownership, not a decoder.
#include "Texture.h"
#include "FileBuffer.h"
#include "Game.h"

bool Game::debugOutput = false;
bool Game::UnsafeMode = false;
bool Game::writeEnabled = true;
int Game::textureQuality = 1;

Texture::Texture() = default;
Texture::~Texture() { delete[] imageData; delete[] tex; }
FileBuffer::FileBuffer(unsigned char *bytes, int size) : length(size), data(bytes) {}
FileBuffer::~FileBuffer() { delete[] data; }

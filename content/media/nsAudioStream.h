/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla code.
 *
 * The Initial Developer of the Original Code is the Mozilla Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2007
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  Chris Double <chris.double@double.co.nz>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */
#if !defined(nsAudioStream_h_)
#define nsAudioStream_h_

#include "nscore.h"
#include "nsISupportsImpl.h"
#include "nsIThread.h"
#include "nsAutoPtr.h"

class nsAudioStream : public nsISupports
{
public:

  enum SampleFormat
  {
    FORMAT_U8,
    FORMAT_S16_LE,
    FORMAT_FLOAT32
  };

  nsAudioStream()
    : mRate(0),
      mChannels(0),
      mFormat(FORMAT_S16_LE)
  {}

  virtual ~nsAudioStream();

  // Initialize Audio Library. Some Audio backends require initializing the
  // library before using it.
  static void InitLibrary();

  // Shutdown Audio Library. Some Audio backends require shutting down the
  // library after using it.
  static void ShutdownLibrary();

  // Thread that is shared between audio streams.
  // This may return null in the child process
  nsIThread *GetThread();

  // AllocateStream will return either a local stream or a remoted stream
  // depending on where you call it from.  If you call this from a child process,
  // you may receive an implementation which forwards to a compositing process.
  static nsAudioStream* AllocateStream();

  // Initialize the audio stream. aNumChannels is the number of audio
  // channels (1 for mono, 2 for stereo, etc) and aRate is the sample rate
  // (22050Hz, 44100Hz, etc).
  // Unsafe to call with the decoder monitor held.
  virtual nsresult Init(PRInt32 aNumChannels, PRInt32 aRate, SampleFormat aFormat) = 0;

  // Closes the stream. All future use of the stream is an error.
  // Unsafe to call with the decoder monitor held.
  virtual void Shutdown() = 0;

  // Write audio data to the audio hardware.  aBuf is an array of frames in
  // the format specified by mFormat of length aCount.  If aFrames is larger
  // than the result of Available(), the write will block until sufficient
  // buffer space is available.
  virtual nsresult Write(const void* aBuf, PRUint32 aFrames) = 0;

  // Return the number of audio frames that can be written without blocking.
  virtual PRUint32 Available() = 0;

  // Set the current volume of the audio playback. This is a value from
  // 0 (meaning muted) to 1 (meaning full volume).
  virtual void SetVolume(double aVolume) = 0;

  // Block until buffered audio data has been consumed.
  // Unsafe to call with the decoder monitor held.
  virtual void Drain() = 0;

  // Pause audio playback
  virtual void Pause() = 0;

  // Resume audio playback
  virtual void Resume() = 0;

  // Return the position in microseconds of the audio frame being played by
  // the audio hardware.
  virtual PRInt64 GetPosition() = 0;

  // Return the position, measured in audio frames played since the stream
  // was opened, of the audio hardware.
  virtual PRInt64 GetPositionInFrames() = 0;

  // Returns true when the audio stream is paused.
  virtual bool IsPaused() = 0;

  // Returns the minimum number of audio frames which must be written before
  // you can be sure that something will be played.
  // Unsafe to call with the decoder monitor held.
  virtual PRInt32 GetMinWriteSize() = 0;

  int GetRate() { return mRate; }
  int GetChannels() { return mChannels; }
  SampleFormat GetFormat() { return mFormat; }

protected:
  nsCOMPtr<nsIThread> mAudioPlaybackThread;
  int mRate;
  int mChannels;
  SampleFormat mFormat;
};

#endif

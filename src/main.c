#include <pd_api.h>
#include <opusfile.h>
#include "main.h"

#define REFRESH_RATE 30
#define MARGIN 10
#define BUFFER_SIZE 11520
#define SAMPLE_RATE 48000
const char instructions[] = "Press A to load from memory, or press B\nto load from filesystem callbacks.";
const char file_path[] = "28.opus";
const char decoding_memory[] = "Decoding from memory...";
const char decoding_callbacks[] = "Decoding from filesystem callbacks...";

PlaydateAPI *pd = NULL;
OpusFileCallbacks opus_file_callbacks;
int16_t scratch_buffer[BUFFER_SIZE];

unsigned char *file_buffer = NULL;
OggOpusFile *opus_file = NULL;
float measured_time = 0;
int total_samples = 0;

// Responds to button input.
int button_loop(void *userdata)
{
    PDButtons released;
    pd->system->getButtonState(NULL, &released, NULL);
    // Load file from memory if A pressed
    if (released & kButtonA)
    {
        return load(0);
    }
    // Load file with callbacks if B pressed
    else if (released & kButtonB)
    {
        return load(1);
    }

    return 0;
}

// Opens the Opusfile stream.
int load(int use_callbacks)
{
    // Get file size and other info
    FileStat file_stat;
    if (pd->file->stat(file_path, &file_stat))
    {
        pd->system->error("%s", pd->file->geterr());
        return 0;
    }

    // Open file
    SDFile *file = pd->file->open(file_path, kFileRead);
    if (file == NULL)
    {
        pd->system->error("%s", pd->file->geterr());
        return 0;
    }

    if (use_callbacks)
    {
        // Open Opusfile stream
        int err;
        opus_file = op_open_callbacks(file, &opus_file_callbacks, NULL, 0, &err);
        if (err)
        {
            pd->system->error("Opusfile error upon opening file callbacks: %i", err);
            return 0;
        }
    }
    else
    {
        // Allocate buffer for file
        file_buffer = pd->system->realloc(NULL, file_stat.size);
        if (file_buffer == NULL)
        {
            pd->system->error("File buffer allocation failed");
            return 0;
        }

        // Read file into buffer
        int bytes_read = pd->file->read(file, file_buffer, file_stat.size);
        if (bytes_read != file_stat.size)
        {
            pd->system->error("File read expected %i bytes, got %i instead", file_stat.size, bytes_read);
            return 0;
        }

        // Close file
        pd->file->close(file);

        // Open Opusfile stream
        int err;
        opus_file = op_open_memory(file_buffer, bytes_read, &err);
        if (err)
        {
            pd->system->error("Opusfile error upon opening memory: %i", err);
            return 0;
        }
    }

    // Clear screen
    pd->graphics->clear(kColorWhite);

    // Draw status to screen
    const char *status = use_callbacks ? decoding_callbacks : decoding_memory;
    pd->graphics->drawText(status, strlen(status), kUTF8Encoding, MARGIN, MARGIN);

    // Switch to decode loop, disable framerate limit
    pd->system->setUpdateCallback(decode_loop, NULL);
    pd->display->setRefreshRate(0);

    return 1;
}

int decode_loop(void *userdata)
{
    // Ensure Opusfile stream is open
    if (opus_file == NULL)
    {
        pd->system->error("Opusfile stream is null while decoding");
        return 0;
    }

    // Start time
    pd->system->resetElapsedTime();

    // Read samples
    int sample_count = op_read_stereo(opus_file, scratch_buffer, BUFFER_SIZE);

    // Increment time
    measured_time += pd->system->getElapsedTime();

    if (sample_count < 0)
    {
        // Error reading Opusfile stream
        pd->system->error("Opusfile error while decoding: %i", sample_count);
        return 0;
    }
    else if (sample_count > 0)
    {
        // Increment total sample count
        total_samples += sample_count;
        return 0;
    }
    else
    {
        // Free Opusfile stream
        op_free(opus_file);
        opus_file = NULL;

        // Free buffer if loaded
        if (file_buffer != NULL)
        {
            pd->system->realloc(file_buffer, 0);
            file_buffer = NULL;
        }

        // Clear screen
        pd->graphics->clear(kColorWhite);

        // Draw result to screen
        float seconds = (float)total_samples / SAMPLE_RATE;
        float speed = seconds / measured_time;
        char *result;
        int result_len = pd->system->formatString(&result, "Decoded %i samples (%i seconds)\nin %f measured seconds\n(%.2fx real-time speed).",
                                                  total_samples, (int)seconds, (double)measured_time, (double)speed);
        pd->graphics->drawText(result, result_len, kUTF8Encoding, MARGIN, MARGIN);
        pd->system->realloc(result, 0);
        total_samples = 0;
        measured_time = 0;

        // Switch to button loop, enable framerate limiter
        pd->system->setUpdateCallback(button_loop, NULL);
        pd->display->setRefreshRate(REFRESH_RATE);

        return 1;
    }
}

// Does any initialization tasks that must run during the update loop.
int init_loop(void *userdata)
{
    pd->graphics->drawText(instructions, strlen(instructions), kUTF8Encoding, MARGIN, MARGIN);
    pd->system->setUpdateCallback(button_loop, NULL);
    return 1;
}

// Adapted seek/tell functions for use with Opusfile
int seek_64(void *_stream, opus_int64 _offset, int _whence)
{
    return pd->file->seek(_stream, (int)_offset, _whence);
};

opus_int64 tell_64(void *_stream)
{
    return pd->file->tell(_stream);
};

#ifdef _WINDLL
__declspec(dllexport)
#endif
int eventHandler(PlaydateAPI* playdate, PDSystemEvent event, uint32_t arg)
{
    if (event == kEventInit)
    {
        pd = playdate;
        opus_file_callbacks = (OpusFileCallbacks) {(op_read_func)pd->file->read, seek_64, tell_64, pd->file->close};
        pd->system->setUpdateCallback(init_loop, NULL);
        pd->display->setRefreshRate(REFRESH_RATE);
    }

    return 0;
}

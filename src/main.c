#include <pd_api.h>
#include <opusfile.h>

const char instructions[] = "Press A to load from memory.";
const char file_path[] = "selection.opus";

int update(void *userdata)
{
    PlaydateAPI *playdate = userdata;

    PDButtons released;
    playdate->system->getButtonState(NULL, &released, NULL);
    // Load file from memory if A pressed
    if (released & kButtonA)
    {
        // Clear screen, start time
        playdate->graphics->clear(kColorWhite);
        playdate->system->resetElapsedTime();

        // Get file size and other info
        FileStat file_stat;
        if (playdate->file->stat(file_path, &file_stat))
        {
            playdate->system->error("%s", playdate->file->geterr());
            return 0;
        }

        // Open file
        SDFile *file = playdate->file->open(file_path, kFileRead);
        if (file == NULL)
        {
            playdate->system->error("%s", playdate->file->geterr());
            return 0;
        }

        // Allocate buffer for file
        unsigned char *file_buffer = playdate->system->realloc(NULL, file_stat.size);
        if (file_buffer == NULL)
        {
            playdate->system->error("File buffer allocation failed");
            return 0;
        }

        // Read file into buffer
        int bytes_read = playdate->file->read(file, file_buffer, file_stat.size);
        if (bytes_read != file_stat.size)
        {
            playdate->system->error("File read expected %i bytes, got %i instead", file_stat.size, bytes_read);
            return 0;
        }

        // Close file
        playdate->file->close(file);

        // Open Opusfile stream
        int err;
        OggOpusFile *opus_file = op_open_memory(file_buffer, bytes_read, &err);
        if (err)
        {
            playdate->system->error("Opusfile error upon opening memory: %i", err);
            return 0;
        }

        // Get stream's sample count
        unsigned int sample_count = op_pcm_total(opus_file, -1);

        // Free Opusfile stream
        op_free(opus_file);

        // Free buffer
        playdate->system->realloc(file_buffer, 0);

        // Get time
        float elapsed_time = playdate->system->getElapsedTime();

        // Draw sample count to screen
        char *samples;
        int samples_len = playdate->system->formatString(&samples, "Duration: %u samples", sample_count);
        playdate->graphics->drawText(samples, samples_len, kUTF8Encoding, 0, 0);
        playdate->system->realloc(samples, 0);

        // Draw time to screen
        char *time;
        int time_len = playdate->system->formatString(&time, "Took %f seconds.", (double)elapsed_time);
        playdate->graphics->drawText(time, time_len, kUTF8Encoding, 0, 100);
        playdate->system->realloc(time, 0);

        return 1;
    }

    return 0;
}

#ifdef _WINDLL
__declspec(dllexport)
#endif
int eventHandler(PlaydateAPI* playdate, PDSystemEvent event, uint32_t arg)
{
    if (event == kEventInit)
    {
        playdate->system->setUpdateCallback(update, playdate);

        playdate->graphics->drawText(instructions, strlen(instructions), kUTF8Encoding, 0, 0);
    }

    return 0;
}

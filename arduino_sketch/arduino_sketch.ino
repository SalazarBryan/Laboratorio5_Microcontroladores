#include <PDM.h>
#include <Laboratorio5_inferencing.h>

#define SLICES_PER_WINDOW 2
#define NOISE_LABEL "NOISE"
#define MIN_CORRECT_PREDICTION 0.8f

/** Audio buffers, pointers, and selectors */
typedef struct {
    signed short *buffers[2];
    unsigned char buf_select;
    unsigned char buf_ready;
    unsigned int buf_count;
    unsigned int n_samples;
} InferenceData;

static InferenceData inference;
static bool isRecordingReady = false;
static signed short *audioBuffer;
static bool enableDebug = false;
static int printResultsCounter = -(SLICES_PER_WINDOW);

// Callback function when PDM data is ready
static void pdmDataReadyCallback(void)
{
    int bytesAvailable = PDM.available();

    // read into the sample buffer
    int bytesRead = PDM.read((char *)&audioBuffer[0], bytesAvailable);

    if (isRecordingReady == true) {
        for (int i = 0; i < bytesRead >> 1; i++) {
            inference.buffers[inference.buf_select][inference.buf_count++] = audioBuffer[i];

            if (inference.buf_count >= inference.n_samples) {
                inference.buf_select ^= 1;
                inference.buf_count = 0;
                inference.buf_ready = 1;
            }
        }
    }
}

// Function to initialize the microphone for audio inference
static void initMicrophoneInference(uint32_t numSamples)
{
    inference.buffers[0] = (signed short *)malloc(numSamples * sizeof(signed short));
    inference.buffers[1] = (signed short *)malloc(numSamples * sizeof(signed short));

    audioBuffer = (signed short *)malloc((numSamples >> 1) * sizeof(signed short));

    inference.buf_select = 0;
    inference.buf_count = 0;
    inference.n_samples = numSamples;
    inference.buf_ready = 0;

    // configure the data receive callback
    PDM.onReceive(&pdmDataReadyCallback);
    PDM.setBufferSize((numSamples >> 1) * sizeof(int16_t));

    // initialize PDM with:
    // - one channel (mono mode)
    // - a 16 kHz sample rate
    if (!PDM.begin(1, EI_CLASSIFIER_FREQUENCY)) {
        ei_printf("Failed to start PDM!");
    }

    // set the gain, defaults to 20
    PDM.setGain(20);

    isRecordingReady = true;
}

// Function to wait until the buffer is ready for recording
static void waitForRecordingReady(void)
{
    while (inference.buf_ready == 0) {
        delay(1);
    }
    inference.buf_ready = 0;
}

// Function to convert audio data from the inference buffer to floating-point format
static int getAudioData(size_t offset, size_t length, float *out_ptr)
{
    numpy::int16_to_float(&inference.buffers[inference.buf_select ^ 1][offset], out_ptr, length);

    return 0;
}

// Function to clean up the microphone and free allocated memory
static void endMicrophoneInference(void)
{
    PDM.end();
    free(inference.buffers[0]);
    free(inference.buffers[1]);
    free(audioBuffer);
}

void setup()
{
    // Initialize serial communication
    Serial.begin(115200);

    // Wait for USB connection (needed for native USB)
    while (!Serial);

    // Print inferencing settings
    ei_printf("Inferencing settings:\n");
    ei_printf("\tInterval: %.2f ms.\n", (float)EI_CLASSIFIER_INTERVAL_MS);
    ei_printf("\tFrame size: %d\n", EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE);
    ei_printf("\tSample length: %d ms.\n", EI_CLASSIFIER_RAW_SAMPLE_COUNT / 16);
    ei_printf("\tNo. of classes: %d\n", sizeof(ei_classifier_inferencing_categories) /
                                            sizeof(ei_classifier_inferencing_categories[0]));

    // Initialize classifier and microphone
    run_classifier_init();
    initMicrophoneInference(EI_CLASSIFIER_SLICE_SIZE);
}

void loop()
{
    // Record audio
    waitForRecordingReady();

    // Set up the signal structure
    signal_t signal;
    signal.total_length = EI_CLASSIFIER_SLICE_SIZE;
    signal.get_data = &getAudioData;
    ei_impulse_result_t result = {0};

    // Run continuous classification
    EI_IMPULSE_ERROR r = run_classifier_continuous(&signal, &result, false);

    if (++printResultsCounter >= (SLICES_PER_WINDOW)) {
        // Print the predictions
        ei_printf("Predictions ");
        ei_printf("(DSP: %d ms., Classification: %d ms., Anomaly: %d ms.)",
            result.timing.dsp, result.timing.classification, result.timing.anomaly);
        ei_printf(": \n");
        for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
            if (result.classification[ix].label != NOISE_LABEL &&
                result.classification[ix].value > MIN_CORRECT_PREDICTION) {
                ei_printf("Detected %s: %.5f\n", result.classification[ix].label,
                          result.classification[ix].value);
                Serial.write(result.classification[ix].label);
            }
        }
        printResultsCounter = 0;
    }
}
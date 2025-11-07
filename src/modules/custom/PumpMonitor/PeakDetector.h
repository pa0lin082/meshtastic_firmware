class PeakDetector
{
  private:
    float peakThreshold;

  public:
    bool detectPeak(float current)
    {
        static float lastValue = 0;

        // Rileva picchi improvvisi
        if (abs(current - lastValue) > peakThreshold) {
            LOG_WARN("Pump: Sudden current spike detected");
            return true;
        }

        lastValue = current;
        return false;
    }
};
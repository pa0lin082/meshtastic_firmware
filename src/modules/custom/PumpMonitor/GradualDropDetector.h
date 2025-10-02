class GradualDropDetector
{
  private:
    float *history;
    int historyIndex;

  public:
    bool detectGradualDrop()
    {
        // Calcola trend lineare
        float slope = calculateSlope(history, 50);

        // Se il trend è negativo e significativo
        if (slope < -0.01) { // 0.01A/secondo
            return true;     // Calo graduale rilevato
        }
        return false;
    }
};
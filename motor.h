class Motor 
{
  private:
    int ConnectSocket;
    double dutyCycle;
    double wantedFreq;
    unsigned int wantedPeriod;

  public:
     Motor(void);
    ~Motor(void);
     void setDutyCycle(double dutyCycle);
     void setWantedFreq(double freq) { wantedFreq = freq; wantedPeriod = 1e6 / freq + 0.5; };
     double getWantedFreq(void) { return wantedFreq; };
     double getDutyCycle(void) { return dutyCycle; };
     void control(unsigned int rotationPeriod_us);
     void init(void);
};     
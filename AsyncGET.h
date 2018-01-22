class TaskAsyncGet : public Task
{
public:
    TaskAsyncGet(IPAddress server, String URL, uint32_t timeInterval) : // pass any custom arguments you need
        Task(timeInterval),
        connectTo(server),
        ledOn(false)  // initialize members here
    { };

private:
    // put member variables here that are scoped to this object
    IPAddress connectTo;
    bool timedOut;
    const uint8_t url; // const means can't change other than in constructor

    virtual bool OnStart() // optional
    {
        // put code here that will be run when the task starts
        ledOn = false;
        pinMode(ledPin, OUTPUT);
        return true;
    }

    virtual void OnStop() // optional
    {
        // put code here that will be run when the task stops
        ledOn = false;
        digitalWrite(ledPin, LOW);    // turn the LED off by making the voltage LOW
    }

    virtual void OnUpdate(uint32_t deltaTime)
    {
        if (ledOn)
        {
            digitalWrite(ledPin, LOW);    // turn the LED off by making the voltage LOW
        }
        else
        {
            digitalWrite(ledPin, HIGH);   // turn the LED on (HIGH is the voltage level)
        }

        ledOn = !ledOn; // toggle led state
    }
};

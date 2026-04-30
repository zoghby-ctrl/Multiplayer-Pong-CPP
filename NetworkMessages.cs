public struct PlayerInput {
    public uint sequenceNumber; 
    public float horizontal;
    public float vertical;
}

public struct PlayerState {
    public uint lastProcessedInput; 
    public Vector3 position;
    public int score;
}
# Guitar Hero 5 Guitar Neck I2C Format

I2C Address: 0x0D

TWI Freq: 100000hz

Do a 2 byte read from 0x12

first byte: frets, stored in the same way as an xbox 360 controller

second byte: tap bar, below dictionary shows how these values are mapped

```c#
static readonly Dictionary<int, BarButton> _mappings = new Dictionary<int, BarButton>() {
    {0x19, BarButton.Green | BarButton.Yellow},
    {0x1A, BarButton.Yellow},
    {0x2C, BarButton.Green | BarButton.Red | BarButton.Yellow | BarButton.Blue},
    {0x2D, BarButton.Green | BarButton.Yellow | BarButton.Blue},
    {0x2E, BarButton.Red | BarButton.Yellow | BarButton.Blue},
    {0x2F, BarButton.Yellow | BarButton.Blue},
    {0x46, BarButton.Green | BarButton.Red | BarButton.Blue},
    {0x47, BarButton.Green | BarButton.Blue},
    {0x48, BarButton.Red | BarButton.Blue},
    {0x49, BarButton.Blue},
    {0x5F, BarButton.Green | BarButton.Red | BarButton.Yellow | BarButton.Blue |  BarButton.Orange},
    {0x60, BarButton.Green | BarButton.Red | BarButton.Blue | BarButton.Orange},
    {0x61, BarButton.Green | BarButton.Yellow | BarButton.Blue | BarButton.Orange},
    {0x62, BarButton.Green | BarButton.Blue | BarButton.Orange},
    {0x63, BarButton.Red | BarButton.Yellow | BarButton.Blue | BarButton.Orange},
    {0x64, BarButton.Red | BarButton.Blue | BarButton.Orange},
    {0x65, BarButton.Yellow | BarButton.Blue | BarButton.Orange},
    {0x66, BarButton.Blue | BarButton.Orange},
    {0x78, BarButton.Green | BarButton.Red | BarButton.Yellow | BarButton.Orange},
    {0x79, BarButton.Green | BarButton.Red | BarButton.Orange},
    {0x7A, BarButton.Green | BarButton.Yellow | BarButton.Orange},
    {0x7B, BarButton.Green | BarButton.Orange},
    {0x7C, BarButton.Red | BarButton.Yellow | BarButton.Orange},
    {0x7D, BarButton.Red | BarButton.Orange},
    {0x7E, BarButton.Yellow | BarButton.Orange},
    {0x7F, BarButton.Orange},
    {0x95, BarButton.Green},
    {0xB0, BarButton.Green | BarButton.Red},
    {0xCD, BarButton.Red},
    {0xE5, BarButton.Green | BarButton.Red | BarButton.Yellow},
    {0xE6, BarButton.Red | BarButton.Yellow},
};
```
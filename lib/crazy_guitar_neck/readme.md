# Crazy guitar neck

These actually use an off-the-shelf EKT2101 chip for the neck, and there are datasheets available for its protocol: https://www.datasheetbank.com/EKT2101-Datasheet-PDF-ELAN

One issue with these guitars is they don't bother connecting the "TPreqB" line. If they did, we would be able to just wait for that to go high to know we need to poll for new data, which would make polling them a lot faster as without this line, we have to poll for data and the chip does not like being polled fast.
# L-Band Satellite Decoders

A few small projects aimed at decoding various L-Band satellites
### Installation 

Install from source:

```
git clone https://github.com/altillimity/L-Band-Decoders.git
mkdir build && cd build
cmake ../
make
```
Package is also available on the ([AUR](https://aur.archlinux.org/packages/l-band-decoders-git/))

### Dependencies

Some projects will require (or / and) :
- [libccsds](https://github.com/altillimity/libccsds)
- [libcorrect](https://github.com/quiet/libcorrect)
- [libfec](https://github.com/quiet/libfec)
- [libpng](https://github.com/glennrp/libpng) + [zlib](https://github.com/madler/zlib)
- [TCLAP](http://tclap.sourceforge.net/)

The flowcharts require GNU Radio 3.8 or above.

# FengYun-3 A/B/C

**Supported downlink :** AHRPT, 1704.5Mhz (3C 1701.4Mhz)    
**Modulation :** QPSK  
**Symbolrate :** 2.8Mbps (3C 2.6MSPS)  
**Recording bandwidth :** >= 3MSPS, 6MSPS or so preferred 

Program to demultiplex / deframe CCSDS frames from unsynced CADUs.
More info in the dedicated folder.

**Decoding :**
- Record a baseband 
- Demodulate with the FY Demodulator flowchart
- Process the soft symbols with FengYun Decoder   
- Use that file in MetFy3x or run it through the CADU RSynchroderand to continue  
- Run the resulting CADU file through the FengYun VIRR Decoder  

# MetOp A/B/C

**Supported downlink :** AHRPT, 1701.3Mhz  
**Modulation :** QPSK  
**Symbolrate :** 2.33Mbps  
**Recording bandwidth :** >= 3MSPS, 6MSPS or so preferred 

**Decoding :**
- Record a baseband 
- Demodulate with the MetOp Demodulator flowchart
- Process the soft symbols with MetOp Decoder   
- Use that file in MetFy3x or run it through the CADU RSynchroderand to continue  
- Run the resulting CADU file through the MetOp AVHRR Decoder  
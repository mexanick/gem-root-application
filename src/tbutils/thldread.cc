#ifndef __CINT__
#include <iomanip> 
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <sstream>
#include <vector>
#include <cstdint>

#include <TFile.h>
#include <TNtuple.h>
#include <TH2.h>
#include <TProfile.h>
#include <TCanvas.h>
#include <TFrame.h>
#include <TROOT.h>
#include <TSystem.h>
#include <TRandom3.h>
#include <TBenchmark.h>
#include <TInterpreter.h>
#include <TApplication.h>
#include <TString.h>

/**
* ... Threshold Scan ROOT based application, could be used for analisys of XDAQ GEM data ...
*/

/*! \file */
/*! 
  \mainpage Threshold Scan ROOT based application.

  VFAT2 data reading example for Threshold Scan XDAQ GEM application, ROOT based analysis.

  \section Installation

  you can make a data file by XDAQ Threshold Scan appliaction or get example from the CERN web:

  wget https://baranov.web.cern.ch/baranov/xdaq/threshold/vfat2_9/ThresholdScan_Fri_Jan_16_14-17-59_2015.dat <br>
  ln -s ThresholdScan_Fri_Jan_16_14-17-59_2015.dat ThresholdScan.dat

  You need a ROOT code for analysis:

  git clone git@github.com:sergueibaranov/gem-root-application.git <br>

  gem-root-application/scripts/with_root_compile.sh gem-root-application/src/tbutils/thldread.cc

  That is all. You will have a root file with 128 threshold scan histograms for one VFAT2 chip.

  \author Sergey.Baranov@cern.ch
*/

using namespace std;

//! GEM VFAT2 Data class.
/*!
  \brief GEMData
  contents VFAT2 GEM data format 
  \author Sergey.Baranov@cern.ch
*/

class GEMData {
  public:

      //! VFAT2 Channel data.
      /*!
        contents VFAT2 128 channels data in two 64 bits words.
       */
    
      //! GEM Event Data Format (one chip data)
      /*! 
        Uncoding of VFAT2 data for one chip, data format.
        \image html vfat2.data.format.png
        \author Sergey.Baranov@cern.ch
       */
    
      struct VFATData {
        uint16_t BC;      /*!<Banch Crossing number "BC" 16 bits, : 1010:4 (control bits), BC:12 */
        uint16_t EC;      /*!<Event Counter "EC" 16 bits: 1100:4(control bits) , EC:8, Flags:4 */
        uint32_t bxExp;   
        uint16_t bxNum;   /*!<Event Number & SBit, 16 bits : bxNum:6, SBit:6 */
        uint16_t ChipID;  /*!<ChipID 16 bits, 1110:4 (control bits), ChipID:12 */
        uint64_t lsData;  /*!<lsData value, bits from 1to64. */ 
        uint64_t msData;  /*!<msData value, bits from 65to128. */
        double delVT;     /*!<delVT = deviceVT2-deviceVT1, Threshold Scan needs this value. */
        uint16_t crc;     /*!<Checksum number, CRC:16 */
      };    
    
      //! Application header struct
      /*!
        \brief AppHeader contens Threshold scan parameters
       */
    
      struct AppHeader {  
        int minTh;     /*!<minTh minimal threshold value. */ 
        int maxTh;     /*!<maxTh maximal threshold value. */ 
        int stepSize;  /*!<stepSize threshold ste size value. */
      };
    
      //! Print Event, "hex" format.
      /*! 
        Print VFAT2 event.
       */
    
      //
      // Useful printouts 
      //
      void show4bits(uint8_t x) {
        int i;
        const unsigned long unit = 1;
        for(i=(sizeof(uint8_t)*4)-1; i>=0; i--)
          (x & ((unit)<<i))?putchar('1'):putchar('0');
     	//printf("\n");
      }

      bool printVFATdata(int event, const VFATData& vfat){
        if( event<0) return(false);
          cout << "Received tracking data word:" << endl;
          cout << "BC      :: 0x" << std::setfill('0') << std::setw(4) << hex << vfat.BC     << dec << endl;
  	  cout << "EC      :: 0x" << std::setfill('0') << std::setw(4) << hex << vfat.EC     << dec << endl;
          cout << "BxExp   :: 0x" << std::setfill('0') << std::setw(4) << hex << vfat.bxExp  << dec << endl; // do we need here?
          cout << "BxNum   :: 0x" << std::setfill('0') << std::setw(4) << hex << vfat.bxNum  << dec << endl; // do we need here?
  	  cout << "ChipID  :: 0x" << std::setfill('0') << std::setw(4) << hex << vfat.ChipID << dec << endl;
          cout << "<127:64>:: 0x" << std::setfill('0') << std::setw(8) << hex << vfat.msData << dec << endl;
          cout << "<63:0>  :: 0x" << std::setfill('0') << std::setw(8) << hex << vfat.lsData << dec << endl;
          cout << "crc     :: 0x" << std::setfill('0') << std::setw(4) << hex << vfat.crc    << dec <<"\n"<< endl;
        return(true);
      };

      bool printVFATdataBits(int event, const VFATData& vfat){
        if( event<0) return(false);
	  cout << "\nReceived VFAT data word: event " << event << endl;
  
          uint8_t   b1010 = (0xf000 & vfat.BC) >> 12;
          show4bits(b1010); cout << " BC     0x" << hex << (0x0fff & vfat.BC) << dec << endl;
  
          uint8_t   b1100 = (0xf000 & vfat.EC) >> 12;
          uint16_t   EC   = (0x0ff0 & vfat.EC) >> 4;
          uint8_t   Flag  = (0x000f & vfat.EC);
          show4bits(b1100); cout << " EC     0x" << hex << EC << dec << endl; 
          show4bits(Flag);  cout << " Flags " << endl;
  
          uint8_t   b1110 = (0xf000 & vfat.ChipID) >> 12;
          uint16_t ChipID = (0x0fff & vfat.ChipID);
          show4bits(b1110); cout << " ChipID 0x" << hex << ChipID << dec << " " << endl;
  
          cout << "     bxExp  0x" << std::setfill('0') << std::setw(4) << hex << vfat.bxExp << dec << " " << endl;
  	  cout << "     bxNum  0x" << std::setfill('0') << std::setw(2) << hex << ((0xff00 & vfat.bxNum) >> 8) << dec << endl;
  	  cout << "     SBit   0x" << std::setfill('0') << std::setw(2) << hex <<  (0x00ff & vfat.bxNum)       << dec << endl;
          cout << " <127:64>:: 0x" << std::setfill('0') << std::setw(8) << hex << vfat.msData << dec << endl;
          cout << " <63:0>  :: 0x" << std::setfill('0') << std::setw(8) << hex << vfat.lsData << dec << endl;
          cout << "     crc    0x" << hex << vfat.crc << dec << endl;
  
          //cout << " " << endl; show16bits(vfat.EC);
  
          return(true);
      };
  
      //! Print ChipID.
      /*! 
          Print ChipID "hex" number and control bits "1110"
       */
    
      bool PrintChipID(int event, const VFATData& vfat){
        if( event<0 ) return(false);
          cout << "\nevent " << event << endl;
          uint8_t bitsE = ((vfat.ChipID&0xF000)>>12);
          showbits(bitsE);
          cout << hex << "1110 0x0" << ((vfat.ChipID&0xF000)>>12) << " ChipID 0x" << (vfat.ChipID&0x0FFF) << dec << endl;
      };
    
      //! Read 1-128 channels data
      /*!
        reading two 64 bits words (lsData & msData) with data from all channels for one VFAT2 chip 
       */
    
      //! Read GEM event data
      /*!
        reading GEM VFAT2 data (BC,EC,bxNum,ChipID,(lsData & msData), crc.
       */
    
      bool readEvent(ifstream& inpf, int event, VFATData& vfat){
        if(event<0) return(false);
          inpf >> hex >> vfat.BC;
          inpf >> hex >> vfat.EC;
          inpf >> hex >> vfat.bxExp;
          inpf >> hex >> vfat.bxNum;
          inpf >> hex >> vfat.ChipID;
          inpf >> hex >> vfat.lsData;
          inpf >> hex >> vfat.msData;
          inpf >>        vfat.delVT;
          inpf >> hex >> vfat.crc;
        return(true);
      };	  
    
      //! read Threshold scan header.
      /*!
        reading of Threshold Scan setup header
       */
    
      bool readHeader(ifstream& inpf, AppHeader& ah){
        inpf >> ah.minTh;
        inpf >> ah.maxTh;
        inpf >> ah.stepSize;
        return(true);
      };	  
    
      //! showbits function.
      /*!
       show bits function, needs for debugging
       */
    
      void showbits(uint8_t x)
        { int i; 
          for(i=(sizeof(uint8_t)*8)-1; i>=0; i--)
            (x&(1<<i))?putchar('1'):putchar('0');
          printf("\n");
        };
};

//! root function.
/*!
https://root.cern.ch/drupal/content/documentation
*/

TROOT root("",""); // static TROOT object

//! main function.
/*!
C++ any documents
*/

int main(int argc, char** argv)
#else
TFile* thldread(Int_t get=0)
#endif
{ cout<<"---> Main()"<<endl;

#ifndef __CINT__
  TApplication App("App", &argc, argv);
#endif

  GEMData data;
  GEMData::VFATData vfat;
  GEMData::AppHeader  ah;

  int ieventPrint = 20;
  string file="ThresholdScan.dat";

  ifstream inpf(file.c_str());
  if(!inpf.is_open()) {
    cout << "\nThe file: " << file.c_str() << " is missing.\n" << endl;
    return 0;
  };

  /* Threshould Analysis Histograms */
  const TString filename = "thldread.root";

  TFile* hfile = NULL;
  hfile = new TFile(filename,"RECREATE","Threshold Scan ROOT file with histograms");

  // read Scan Header 
  data.readHeader(inpf, ah);

  Int_t nBins = ((ah.maxTh - ah.minTh) + 1)/ah.stepSize;

  cout << " minTh " << ah.minTh << " maxTh " << ah.maxTh << " nBins " << nBins << endl;

  TH1F* histo = new TH1F("allchannels", "Threshold scan for all channels", nBins, (Double_t)ah.minTh-0.5,(Double_t)ah.maxTh+0.5 );

  histo->SetFillColor(48);

  // Create a new canvas.
  TCanvas *c1 = new TCanvas("c1","Dynamic Filling Example",50,50,500,500);

  c1->SetFillColor(42);
  c1->GetFrame()->SetFillColor(21);
  c1->GetFrame()->SetBorderSize(6);
  c1->GetFrame()->SetBorderMode(-1);
  c1->Divide(1,1);

  // Booking of 128 histograms for each VFAT2 channel 
  stringstream histName, histTitle;
  TH1F* histos[128];

  for (unsigned int hi = 0; hi < 128; ++hi) {
    
    histName.clear();
    histName.str(std::string());
    histTitle.clear();
    histTitle.str(std::string());

    histName  << "channel"<<(hi+1);
    histTitle << "Threshold scan for channel "<<(hi+1);
    histos[hi] = new TH1F(histName.str().c_str(), histTitle.str().c_str(), nBins, (Double_t)ah.minTh-0.5,(Double_t)ah.maxTh+0.5);
  }

  Int_t ieventMax=1000000;
  const Int_t kUPDATE = 700;

  for(int ievent=0; ievent<ieventMax; ievent++){

    if(inpf.eof()) break;
    if(!inpf.good()) break;

    data.readEvent(inpf, ievent, vfat);

    // cout << "delVT " << vfat.delVT << " " << dec << (vfat.lsData||vfat.msData) << dec << endl;

    if(ievent < ieventPrint){
      data.printVFATdataBits(ievent, vfat);
      //data.printVFATdata(ievent, vfat);
      //data.PrintChipID(ievent,vfat);
    }

    histo->Fill(vfat.delVT, (vfat.lsData||vfat.msData));

    //I think it would be nice to time this...
    for (int chan = 0; chan < 128; ++chan) {
      if (chan < 64)
	histos[chan]->Fill(vfat.delVT,((vfat.lsData>>chan))&0x1);
      else
	histos[chan]->Fill(vfat.delVT,((vfat.msData>>(chan-64)))&0x1);
    }

    if (ievent%kUPDATE == 0 && ievent != 0) {
      if(ievent < ieventPrint) cout << "event " << ievent << " ievent%kUPDATE " << ievent%kUPDATE << endl;
      c1->cd(1);
      histo->Draw();
      c1->Update();
    }

  }
  inpf.close();

  // Save all objects in this file
  hfile->Write();
  cout<<"=== hfile->Write()"<<endl;

#ifndef __CINT__
     App.Run();
#endif

#ifdef __CINT__
   return hfile;
#else
   return 0;
#endif

}

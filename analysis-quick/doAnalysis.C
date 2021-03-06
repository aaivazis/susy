doAnalysis(float stopMass = 200.0 , string signalRegion = "")
{
    gSystem->Load("/home/users/cgeorge/macros/MiniFWLite/libMiniFWLite.so");
    // load CMS2 core
    gSystem->AddIncludePath(Form("-I%s/CORE", gSystem->Getenv("HOME")));
    // load the necessary files
    gROOT->ProcessLine(".L /home/users/aaivazis/susy/analysis-quick/RPV.cc+");
    gROOT->ProcessLine(".L RPVAnalysis.C++");
  
    gStyle->SetOptStat("em");
  
    // set lumi to 19.5
    RPVAnalysis *analysis = new RPVAnalysis();

    analysis->run(stopMass, signalRegion); 
}


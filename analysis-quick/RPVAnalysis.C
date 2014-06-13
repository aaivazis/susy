#include "RPVAnalysis.h"

// run the analysis
void RPVAnalysis::run(){

    // add the files to their respective chains
    TChain* signal600Chain = new TChain("tree");
    signal600Chain->Add("/home/users/aaivazis/susy/babymaker/babies/signal600.root");

    // add the data file to a chain
    TChain* dataChain = new TChain("tree");
    dataChain->Add("/home/users/aaivazis/susy/babymaker/babies/data.root");

    // add the ttjets file to a chain
    TChain* ttjetsChain = new TChain("tree");
    ttjetsChain->Add("/home/users/aaivazis/susy/babymaker/babies/ttjets.root");
    
    // add the dy file to a chain
    TChain* dyChain = new TChain("tree");
    dyChain->Add("/home/users/aaivazis/susy/babymaker/babies/dy.root");
    
    // add the dy file to a chain
    TChain* zzChain = new TChain("tree");
    zzChain->Add("/home/users/aaivazis/susy/babymaker/babies/zz.root");
    
    // fill the dictionaries with empty histograms
    createHistograms();

    // use the jet correction for this sample
    // fillPlots(signal600Chain, signal600, signalDel);
    // fill the data plots
    // fillPlots(dataChain, data, dataDel);
    // fill the tt plots
    // fillPlots(ttjetsChain, ttjets, ttDel);
    // fill the dy plots
    fillPlots(dyChain, dy, dyDel);
    // fill the zz plots
    // fillPlots(zzChain, zz, zzDel);

    // draw the histograms
    // plotHistograms();
}

// fill the given dictionary with the important quantities
void RPVAnalysis::fillPlots(TChain* chain, map<string, TH1F*> sample, TH2F* plot){

    // get the list of files from the chain
    TObjArray* files = chain->GetListOfFiles();
    // create an iterator over the fiels
    TIter fileIter(files);
    // store the current file data
    TFile* currentFile = 0;

    // stream to write the event list    
    ofstream stream;

    // definitions for signal region calc
    int mets[6]= {20, 40, 60, 80, 100, 1000};
    int jets[4] = {0, 1, 2, 3};

    int counters[6][4] = {0};
    int metnumber = sizeof(mets)/sizeof(mets[0]);
    int jetnumber = sizeof(jets)/sizeof(jets[0]);

    // loop over the files to fill plots
    while(( currentFile = (TFile*)fileIter.Next() )) {
        
        // get the file content
        TFile* file = new TFile(currentFile->GetTitle());
        TTree* tree = (TTree*)file->Get("tree");
        // tell cms2 about the tree
        cms2.Init(tree);

        // declare mumu and emu counters
        int mumuCounter = 0;
        int emuCounter = 0;

        // loop over events in the tree
        for (unsigned int event = 0; event < tree->GetEntriesFast(); ++event) {

            // load the event into the branches
            cms2.GetEntry(event);
           
            //  if (event != 17966) continue;

            // save the delta mass between jet/lepton combos (minimized)
            float deltaMass = 9999;
            // save the average mass of the same pair
            float avgMass = 0;
            // counter for btags
            float nBtags = 0;
            // counter for nJets 
            float nJets = 0;
            
            // store the indices of the jets that minimize deltaMass
            int jetllIndex;
            int jetltIndex;

            // counters and indices for the gen_ps loop
            float generatedMass1 = -1;
            float generatedMass2 = -1;
            int llGenerated = -1;
            int ltGenerated = -1;
            int jetllGenerated = -1;
            int jetltGenerated =-1;
            // store the list of known good indices for the gen_ps loop
            set<int> indices;
            // save if the mass pair is valid
            bool genMassGood = true;


            // loop over the jets to compute delta mass
            for (unsigned int j=0; j<jets_p4().size() ; j++) {

                // check that the jet is "good"
                if (! isGoodJet(j)) continue; 
                nJets++;
                // count number of Btags
                if (btagDiscriminant().at(j) < 0.244) continue;
                nBtags++;

                // save the combined masses from the two jet loops to 
                // calculate average and delta mass
                float mass1 = 0;
                float mass2 = 0;

                // loop over jets to find pairs
                for (unsigned int k = 0; k<jets_p4().size() - 1 ; k++){
                    // ignore the jet from the first loop
                    if (k == j) continue;
                    // make sure the second jet is good too
                    if (! isGoodJet(k)) continue;
                    //if (btagDiscriminant().at(k) < 0.244) continue; Alex doesnt have this 2x 

                    mass1 = (ll_p4() + jets_p4().at(j)).M();
                    mass2 = (lt_p4() + jets_p4().at(k)).M();

                    // minimize the delta mass
                    if (fabs(mass2-mass1) < fabs(deltaMass)){
                        // save the delta and average mass of the minimized pair
                        deltaMass = mass2-mass1;
                        avgMass = (mass1+mass2)/2;
                        // save the indices of the jets that minimized delta mass
                        jetllIndex = j;
                        jetltIndex = k;
                    }
                }
            }

            // perform cuts
            if (type() != 0) continue; //ignore ee
            if (/*type() == 0 && */ fabs((ll_p4()+lt_p4()).M() - 91) < 15) continue; // mumu only z-veto
            if (nBtags < 1) continue; 
            
            for (int i =0; i < metnumber; i++){
                // cout << "minimum met: " << mets.at(i) << endl;
                for (int k = 0; k < jetnumber; k++){
                    // cout << "minimum nJets: " << jets.at(k) << endl;
                    if (met() <= mets[i] && nJets >= jets[k]){  
                        counters[i][k]++;
                    }
                }
            }

            //if (type() ==0 && met() > 60) continue; // mumu only
            if (nJets < 2) continue;
            if (fabs(deltaMass) > 100) continue; 
            if (fabs(avgMass) > 250) continue; 
            

            // find the gen_ps particles corresponding to our p4s
            llGenerated = getMatchingGeneratedIndex(ll_p4(), indices);
            indices.insert(llGenerated);

            ltGenerated = getMatchingGeneratedIndex(lt_p4(), indices);
            indices.insert(ltGenerated);

            jetllGenerated = getMatchingGeneratedIndex(jets_p4().at(jetllIndex) , indices);
            indices.insert(jetllGenerated);

            jetltGenerated = getMatchingGeneratedIndex(jets_p4().at(jetltIndex) , indices);

            // calculate the combined mass of the two pairs
            if (llGenerated != -1 && ltGenerated != -1 && jetllGenerated != -1 && jetltGenerated != -1){
                if (isValidPair(llGenerated, jetllGenerated))
                    generatedMass1 = (generated_p4().at(llGenerated)
                                      + generated_p4().at(jetllGenerated)).M();
                else
                    genMassGood = false;

                if (isValidPair(ltGenerated, jetltGenerated))
                    generatedMass2 = (generated_p4().at(ltGenerated)
                                      + generated_p4().at(jetltGenerated)).M();
                else
                    genMassGood = false;
            }
            
            
            // fill the appopriate plots
            sample["avgMass"]->Fill(avgMass);
            
            if (plot) plot->Fill(avgMass, deltaMass, scale_1fb());

            // increment eventCounters
            if (type() == 0) mumuCounter++;
            if (type() == 1 || type() == 2) emuCounter++;

            stream.open("eventList.txt", ios::app);
            stream << event << endl;
            stream.close();

        }

        // print the event counters
        cout << "number of mumu events: " << mumuCounter * lumi * scale_1fb() << endl;
        cout << "number of emu events: " << emuCounter << endl;

    }

    int counterSize = sizeof(counters)/sizeof(counters[0]);
    
    stream.open("signalregion.txt", ios::app);
    for (int i = 0; i< metnumber; i++){
        for (int k =0; k < jetnumber; k++){
            stream << counters[i][k] * lumi * scale_1fb() << " ";
        }
    }
    stream << endl;
    stream.close();


    return;
}

// utility/background functions

// check if the requested jet is "good"
bool RPVAnalysis::isGoodJet(int index) {
    // pt > 30
    if (jets_p4().at(index).pt() < 30) return false;
    // eta < 2.5
    if (fabs(jets_p4().at(index).eta()) > 2.5)  return false;
    // dR > 0.4
    if (ROOT::Math::VectorUtil::DeltaR(jets_p4().at(index), ll_p4()) < 0.4) return false;
    if (ROOT::Math::VectorUtil::DeltaR(jets_p4().at(index), lt_p4()) < 0.4) return false;

    return true;
}

bool RPVAnalysis::isValidPair(int hypIndex, int jetIndex){

    // require the generated pair to be either mu- b or mu+ bbar
 
    // muons have id = 13
    // b's have id = 5
    // i need bbar and muon (or opposite)
    return generated_id().at(hypIndex) * generated_id().at(jetIndex) == -65;
}

int RPVAnalysis::getMatchingGeneratedIndex(const LorentzVector candidate, set<int> indices){

    float deltaMin = 9999;
    int index = -1;

    for( int i = 0; i < generated_p4().size(); ++i){
        
        // check if i is in indices
        if ( indices.find(i) != indices.end() ) continue;

        // compute deltaR
        float deltaR = ROOT::Math::VectorUtil::DeltaR(generated_p4().at(i), candidate);

        // delta R < 0.1
        if (fabs(deltaR) > 0.1) continue; 

        // grab the minimum
        if (deltaR < deltaMin )  {
            index = i;
            deltaMin = deltaR;
        }
    }  

    // return the index of the generated particle that matches
    return index;
}

// fill the sample dictionaries 
void RPVAnalysis::createHistograms() {
   
    // create signal200 plots
    signal600["avgMass"] = new TH1F("signal600_avgMass", "signal 600 Avg Mass", 240, 0, 1200);
    signal600["genMinusReco"] = new TH1F("signal600_genMinusReco", "generated - reco mass", 100, -75, 75);

    signalDel = new TH2F("signal", "zz", 100, 0, 1200, 100, -200, 200);
    zzDel = new TH2F("zz", "zz", 100, 0, 1200, 100, -200, 200);
    dyDel = new TH2F("dy", "zz", 100, 0, 1200, 100, -200, 200);
    ttDel = new TH2F("tt", "zz", 100, 0, 1200, 100, -200, 200);
    dataDel = new TH2F("data", "zz", 100, 0, 1200, 100, -200, 200);

    // create ttjets plots
    ttjets["avgMass"] = new TH1F("ttjets_avgMass", "ttjets Avg Mass", 240, 0, 1200);
    ttjets["genMinusReco"] = new TH1F("ttjets_genMinusReco", "ttjets generated - reco mass", 100, -75, 75);

    // create dy plots
    dy["avgMass"] = new TH1F("dy_avgMass", "dy Avg Mass", 240, 0, 1200);
    dy["genMinusReco"] = new TH1F("dy_genMinusReco", "dy generated - reco mass", 100, -75, 75);

    // create zz plots
    zz["avgMass"] = new TH1F("zz_avgMass", "dy Avg Mass", 240, 0, 1200);
    zz["genMinusReco"] = new TH1F("zz_genMinusReco", "dy generated - reco mass", 100, -75, 75);

    // create data plots
    data["avgMass"] = new TH1F("data_avgMass", "data Avg Mass", 240, 0, 1200);
    data["genMinusReco"] = new TH1F("data_genMinusReco", "data generated - reco mass", 100, -75, 75);

    return;
}

// give the histograms color,overflow, etc.
void RPVAnalysis::prepareHistograms(){


    // color the histograms
    signal600["avgMass"]->SetLineColor(kBlack);
    ttjets["avgMass"]->SetLineColor(kRed);
    signal600["genMinusReco"]->SetLineColor(kBlack);
    //  signal600before["genMinusReco"]->SetLineColor(kRed);

    signalDel->SetFillColor(kBlack);
    dyDel->SetFillColor(kRed);
    zzDel->SetFillColor(kRed);
    ttDel->SetFillColor(kRed);
    
    // set the overflow bins
    overflow(signal600["avgMass"], 1200);

    return;
}

// set the bin corresponding to {overflowValue} as the histograms overflow bin
void RPVAnalysis::overflow(TH1F *histo, float overflowValue){
    return histo->SetBinContent(histo->GetXaxis()->FindBin(overflowValue), histo->GetBinContent((histo->GetXaxis()->FindBin(overflowValue)))+histo->GetBinContent((histo->GetXaxis()->FindBin(overflowValue))+1));
}

void RPVAnalysis::plotHistograms(){

    // set up canvas and legend 
    TCanvas *c1 = new TCanvas("c1","Graph Example",200,10,700,500);
    //c1->SetLogy();
    
    // create the legend
    TLegend *leg = new TLegend(.73,.9,.89,.6);

    /*
    // add entries to the legend
    leg->AddEntry(ttjets["avgMass"], "ttbar", "f");
    leg->AddEntry(dy["avgMass"] , "dy", "f");
    leg->AddEntry(zz["avgMass"], "zz", "f");
    leg->AddEntry(signal600["avgMass"], "signal600", "l");
    */

    // style the legend
    leg->SetFillStyle(0);
    leg->SetBorderSize(0);
    leg->SetTextSize(0.04);

    // prepare the histograms
    prepareHistograms();

    signalDel->Draw("box");
    dyDel->Draw("samebox");
    zzDel->Draw("samebox");
    ttDel->Draw("samebox");

    /* stacked plots 
    //  THStack *stack = new THStack("stack","");

    
    leg->Draw("same");
    c1->SaveAs("coeff_g2.png");

    c1 = new TCanvas("c1","Graph Example",200,10,700,500);
    leg = new TLegend(.73,.9,.89,.6);

    leg->AddEntry(signal600["avgMass"], "After", "l");
    leg->AddEntry(ttjets["avgMass"], "Before", "l");


    ttjets["avgMass"]->Draw();
    signal600["avgMass"]->Draw("same");
    leg->Draw("same");
    c1->SaveAs("avgMass_g2.png");
    */
}

    
//Pressure drop test script v6
// Mete Yucel 6/1/22
//Usage : root -l "pd2.C(\"YOURLEAKFILE.txt\")"
#include "fit.h"
#include <iterator>

extern TF1 *fitFcn;
inline std::vector<std::string> split(std::string s, char d);


void pd2(std::string fn){
  
  ifstream in1;

  float initial_cut = 0.1;
  float final_cut = 0.9;

  // This can be used to feed plane and panel information
  auto split_name = split(fn,'_');


  in1.open(fn.c_str());
  bool panel0(1);
  bool panel1(1);
  bool panel2(1);
  bool panel3(1);
  bool panel4(1);
  bool panel5(1);

  double p0_, p1_, p2_, p3_, p4_, p5_, pRef_, tRef_;
  int n_;

  auto hp0 = new TH1F("hp0","hp0",100,28.,32.);
  auto hp1 = new TH1F("hp1","hp1",100,28.,32.);
  auto hp2 = new TH1F("hp2","hp2",100,28.,32.);
  auto hp3 = new TH1F("hp3","hp3",100,28.,32.);
  auto hp4 = new TH1F("hp4","hp4",100,28.,32.);
  auto hp5 = new TH1F("hp5","hp5",100,28.,32.);
  std::vector<int> t;
  std::vector<double> tP;
  std::vector<double> p0;
  std::vector<double> p1;
  std::vector<double> p2;
  std::vector<double> p3;
  std::vector<double> p4;
  std::vector<double> p5;
  std::vector<double> pRef;
  std::vector<double> tRef;
  std::vector<double> tEnv;
  std::string time;
  std::string date;
  std::string c_;


  TFile *hfile = hfile = TFile::Open("test.root","RECREATE");
  TTree treePD("pd", "pressure-drop");
  treePD.Branch("p0", &p0);
  treePD.Branch("p1", &p1);
  treePD.Branch("p2", &p2);
  treePD.Branch("p3", &p3);
  treePD.Branch("p4", &p4);
  treePD.Branch("p5", &p5);
  treePD.Branch("pRef", &pRef);
  treePD.Branch("tRef", &tRef);
  treePD.Branch("tEnv", &tEnv);
  treePD.Branch("t",&t);
  treePD.Branch("tP",&tP);

  int line = 0;
  double tRef_old = 0;
  double tRef_current = 0;
  double tP_ = 0;//plane
  double tStraw = 0;
  double tManifold = 0;
  double tEnv_ = 0.;

  //Reference
  const double aRef = 0.07;// in m2, 106.9 in in2
  const double cRef = 490;// J/kgK
  const double mRef = 2.859;// kg measured
  const double hAir_ref = 5000;// W/m2 K, arbitarly set to get env temp change of +/- 0.5 C
  const double Rref = (mRef*cRef)/(hAir_ref*aRef); // 

  //Plane
  const double aP = (521554*2 + 10173)/10E6;//m2 side surface + top surface including straws
  const double cP = 890;//J/kgK Aluminum
  const double mP = 45.; //kg Weight of the plane
  const double hAir_plane = 13.7;// W/m2 K, P. Kosky, G. Wise, Convection Heat Transfer Coefficient 2013
  const double Rp = (mP*cP)/(hAir_plane*aP);

  //Straws
  const double aS = (1153950*2)/10E6;//m2 top and bottom straws of the plane
  const double cS = 1170;//J/kgK Mylar 
  const double mS = 31.1/1000.; //kg weight of the straws, 1 m ave length is taken - Mylar density 1.38 g/cm3
  const double RS = (mS*cS)/(hAir_plane*aS);
  

  std::vector<std::string> sqlDateTime;
  while(!in1.eof()){
    line++;
    //std::cout<<line<<std::endl;
    in1 >> date >> time >> n_ >> p0_ >> p1_ >> p2_ >> p3_ >> p4_ >> p5_ >> pRef_ >> tRef_;
    tRef_ = tRef_;
    tRef_current = tRef_;
    auto date_tokens = split(date, '-');
    sqlDateTime.push_back(date_tokens[2] + "-" + date_tokens[0] + "-"  + date_tokens[1] + " " + time);//yyyy-mm-dd
    if(line>1){

      tEnv_ = (tRef_current-(tRef_old*exp(-1/Rref)))/(1 - exp(-1/Rref));
      //tMan is under estimated due to low Rp and high tEnv!!!!
      tManifold = tEnv_ + (tManifold-tEnv_)*exp(-1/(Rp));
      tStraw = tEnv_ + (tStraw-tEnv_)*exp(-1/(RS));
      tP_ = (tManifold + tStraw) /2.; //Straw and panel gas volume is approx same
      tRef_old = tRef_current;

    }
    else {
      tRef_ = tRef_;
      tRef_old = tRef_;
      tStraw = tRef_ ;
      tManifold = tRef_;
      tP_ = tRef_;
    }


    const double mbar_psi =  -0.0145;
    double offset = 29.5;
    p0.push_back(p0_*mbar_psi + offset);hp0->Fill(p0_*mbar_psi );
    p1.push_back(p1_*mbar_psi + offset);hp1->Fill(p1_*mbar_psi );
    p2.push_back(p2_*mbar_psi + offset);hp2->Fill(p2_*mbar_psi );
    p3.push_back(p3_*mbar_psi + offset);hp3->Fill(p3_*mbar_psi );
    p4.push_back(p4_*mbar_psi + offset);hp4->Fill(p4_*mbar_psi );
    p5.push_back(p5_*mbar_psi + offset);hp5->Fill(p5_*mbar_psi );
    pRef.push_back(pRef_);
    tEnv.push_back(tEnv_);
    tRef.push_back(tRef_);
    tP.push_back(tP_);


    auto split_time = split(time,':');
    auto split_date = split(date,'-');
    TDatime * tt ;  
    tt = new TDatime(std::stoi(split_date[2]),std::stoi(split_date[0]),std::stoi(split_date[1]),std::stoi(split_time[0]),std::stoi(split_time[1]),std::stoi(split_time[2]));
    t.push_back(tt->Convert());
    split_time.clear();
  }

  //create Root Tree from std::vector to help with data cuts
  treePD.Fill();
  treePD.Print();
  treePD.Write();

  int tempi = int(sqlDateTime.size()*initial_cut);
  int tempf = int(sqlDateTime.size()*final_cut);
  int tempi2 = int(sqlDateTime.size()*(initial_cut+0.05));// For the P-T calculation
  auto ti = new TDatime(sqlDateTime.at(tempi).c_str());
  auto tf = new TDatime(sqlDateTime.at(tempf).c_str());
  auto ti2 = new TDatime(sqlDateTime.at(tempi2).c_str());// For the P-T calculation

  std::cout<<"Pressure drop start time:"<<std::endl;
  ti->Print();
  std::cout<<"Pressure drop end time:"<<std::endl;
  tf->Print();

  // Cut for pressure drop test, remove final and initial hours
  string tcut_str = string("t > ") + std::to_string(ti->Convert()) + string(" && t < ") + std::to_string(tf->Convert());;// Change tf here to make P-T correection smaller
  TCut tcut = tcut_str.c_str();
  // Cut for pressure vs temperature(P-T) determination, use a short duration after the initial cut
  string tcut_str2 = string("t > ") + std::to_string(ti->Convert()) + string(" && t < ") + std::to_string(ti2->Convert());;// Change tf here to make P-T correection smaller
  TCut tcut2 = tcut_str2.c_str();

  
  //////////// P-T CUT START //////////
  //clone original for the P-T cut
  auto cloneTree = treePD.CloneTree(); // Clone it for a different cut
  //apply cuts to data;
  cloneTree->Draw(">>myList",tcut2,"entrylist");
  TEntryList *list2 = (TEntryList*)gDirectory->Get("myList");
  list2->SetReapplyCut(kTRUE);//this line is critical to apply the cut to the vectors inside tree branches
  std::cout<<"number of entries after cut 1 = "<<list2->GetN()<<std::endl;

  cloneTree->SetEntryList(list2);

  // Draw trees and retrieve TGraphs. NOTE: it you use >> TH2F is returned.
  TGraph * temp;
  cloneTree->Draw("pRef:tRef"); temp = (TGraph*)gPad->GetPrimitive("Graph"); auto gRef = (TGraph*)temp->Clone("gRef");
  cloneTree->Draw("p0:tP"); temp = (TGraph*)gPad->GetPrimitive("Graph"); auto g0 = (TGraph*)temp->Clone("g0");
  cloneTree->Draw("p1:tP"); temp = (TGraph*)gPad->GetPrimitive("Graph"); auto g1 = (TGraph*)temp->Clone("g1");
  cloneTree->Draw("p2:tP"); temp = (TGraph*)gPad->GetPrimitive("Graph"); auto g2 = (TGraph*)temp->Clone("g2");
  cloneTree->Draw("p3:tP"); temp = (TGraph*)gPad->GetPrimitive("Graph"); auto g3 = (TGraph*)temp->Clone("g3");
  cloneTree->Draw("p4:tP"); temp = (TGraph*)gPad->GetPrimitive("Graph"); auto g4 = (TGraph*)temp->Clone("g4");
  cloneTree->Draw("p5:tP"); temp = (TGraph*)gPad->GetPrimitive("Graph"); auto g5 = (TGraph*)temp->Clone("g5");
  delete temp;
  //////////// P-T CUT END //////////


  /////////// PRESSURE DROP CUT START /////////////
  //apply cuts to data;
  treePD.Draw(">>myList",tcut,"entrylist");
  TEntryList *list=(TEntryList*)gDirectory->Get("myList");
  list->SetReapplyCut(kTRUE);//this line is critical to apply the cut to the vectors inside tree branches
  std::cout<<"number of entries after cut 1 = "<<list->GetN()<<std::endl;

  treePD.SetEntryList(list);

  //same thing but do it on a fake canvas
  auto cfake = new TCanvas("cfake","cfake",800,600);
  cfake->cd();
  TGraph * temp2;
  treePD.Draw("pRef:t"); temp2 = (TGraph*)gPad->GetPrimitive("Graph"); auto grRef = (TGraph*)temp2->Clone("grRef");
  treePD.Draw("p0:t"); temp2 = (TGraph*)gPad->GetPrimitive("Graph"); auto gr0 = (TGraph*)temp2->Clone("gr0");
  treePD.Draw("p1:t"); temp2 = (TGraph*)gPad->GetPrimitive("Graph"); auto gr1 = (TGraph*)temp2->Clone("gr1");
  treePD.Draw("p2:t"); temp2 = (TGraph*)gPad->GetPrimitive("Graph"); auto gr2 = (TGraph*)temp2->Clone("gr2");
  treePD.Draw("p3:t"); temp2 = (TGraph*)gPad->GetPrimitive("Graph"); auto gr3 = (TGraph*)temp2->Clone("gr3");
  treePD.Draw("p4:t"); temp2 = (TGraph*)gPad->GetPrimitive("Graph"); auto gr4 = (TGraph*)temp2->Clone("gr4");
  treePD.Draw("p5:t"); temp2 = (TGraph*)gPad->GetPrimitive("Graph"); auto gr5 = (TGraph*)temp2->Clone("gr5");
  treePD.Draw("tRef:t"); temp2 = (TGraph*)gPad->GetPrimitive("Graph"); auto grt = (TGraph*)temp2->Clone("grt");
  treePD.Draw("tP:t"); temp2 = (TGraph*)gPad->GetPrimitive("Graph"); auto grtP = (TGraph*)temp2->Clone("grtP");
  treePD.Draw("tEnv:t"); temp2 = (TGraph*)gPad->GetPrimitive("Graph"); auto grtE= (TGraph*)temp2->Clone("grtE");
  delete temp2;
  cfake->Close();

  /////////// PRESSURE DROP CUT END /////////////



  //Initial and final temperature guess for plane and ref temperature
  TF1 *f1 = new TF1("f1","pol1");
  f1->SetLineColor(kGreen);  f1->SetLineStyle(kDashDotted);f1->SetLineWidth(4);
  grt->Fit(f1);
  double TiRef = f1->Eval(ti->Convert());
  double TfRef = f1->Eval(tf->Convert());
  f1->SetLineColor(kBlue);  f1->SetLineStyle(kDashDotted);f1->SetLineWidth(4);
  grtP->Fit(f1);
  double TiPlane = f1->Eval(ti->Convert());
  double TfPlane = f1->Eval(tf->Convert());
  f1->SetLineColor(kRed);  f1->SetLineStyle(kDashDotted);f1->SetLineWidth(4);
  grtE->Fit(f1);
  double TiEnv = f1->Eval(ti->Convert());
  double TfEnv = f1->Eval(tf->Convert());

  //print info  
  auto deltat = tf->Convert() - ti->Convert();
  string tif= string("t==") + std::to_string(ti->Convert()) + string(" || t==") + std::to_string(tf->Convert());
  std::cout<<"pressure drop test length = "<<deltat<<std::endl;
  treePD.Scan("pRef:p0:p1:p2:p3:p4:p5:tRef:t",tif.c_str(),"colsize=11");
  
  //calculate pressure difference
  auto it_i = std::find(t.begin(),t.end(),ti->Convert());
  auto i_i = std::distance(t.begin(),it_i);
  auto it_f = std::find(t.begin(),t.end(),tf->Convert());// Change tf here to make P-T correection smaller
  auto i_f = std::distance(t.begin(),it_f);
  double tRef_i = tRef.at(i_i);
  double tRef_f = tRef.at(i_f);
  double tP_i = tP.at(i_i);
  double tP_f = tP.at(i_f);
  double deltap1 = p1.at(i_i) - p1.at(i_f);
  double deltap0 = p0.at(i_i) - p0.at(i_f);
  double deltap2 = p2.at(i_i) - p2.at(i_f);
  double deltap4 = p4.at(i_i) - p4.at(i_f);
  double deltapRef = pRef.at(i_i) - pRef.at(i_f);
  double deltap3 = p3.at(i_i) - p3.at(i_f);
  double deltap5 = p5.at(i_i) - p5.at(i_f);

  auto cPoints = new TCanvas("cPoints","cPoints",1200,900);
  auto gPoints = new TGraph(2);
  cPoints->cd();
  gPoints->SetPoint(0,ti->Convert(),p0.at(i_i));
  gPoints->SetPoint(1,tf->Convert(),p0.at(i_f));
  gPoints->SetMarkerStyle(kOpenSquare);
  gPoints->SetMarkerColor(kBlue);
  gPoints->SetMarkerSize(2);
  cPoints->Close();

  auto cTree = new TCanvas("cTree","cTree",1200,900);
  cTree->Divide(3,3);
  gRef->SetTitle("P_{ref} vs T_{ref}");
  g0->SetTitle("P_{0} vs T_{plane}");
  g1->SetTitle("P_{1} vs T_{plane}");
  g2->SetTitle("P_{2} vs T_{plane}");
  g3->SetTitle("P_{3} vs T_{plane}");
  g4->SetTitle("P_{4} vs T_{plane}");
  g5->SetTitle("P_{5} vs T_{plane}");
  cTree->cd(2); gRef->Draw("AP"); gRef->Fit(f1);double pRefCorr = f1->Eval(TfRef) - f1->Eval(TiRef);
  cTree->cd(4); g0->Draw("AP"); g0->Fit(f1);double p0Corr = f1->Eval(TfPlane) - f1->Eval(TiPlane);
  cTree->cd(5); g1->Draw("AP"); g1->Fit(f1);double p1Corr = f1->Eval(TfPlane) - f1->Eval(TiPlane);
  cTree->cd(6); g2->Draw("AP"); g2->Fit(f1);double p2Corr = f1->Eval(TfPlane) - f1->Eval(TiPlane);
  cTree->cd(7); g3->Draw("AP"); g3->Fit(f1);double p3Corr = f1->Eval(TfPlane) - f1->Eval(TiPlane);
  cTree->cd(8); g4->Draw("AP"); g4->Fit(f1);double p4Corr = f1->Eval(TfPlane) - f1->Eval(TiPlane);
  cTree->cd(9); g5->Draw("AP"); g5->Fit(f1);double p5Corr = f1->Eval(TfPlane) - f1->Eval(TiPlane);

 

  auto c1 = new TCanvas("c1","c1",800,600);
  c1->cd();
  f1->SetLineColor(kGreen);  f1->SetLineStyle(kDashDotted);f1->SetLineWidth(4);
  grRef->Draw("AL");grRef->Fit(f1);double deltapRef_new = f1->Eval(tf->Convert()) - f1->Eval(ti->Convert());
  grRef->GetXaxis()->SetTimeDisplay(1);
  grRef->GetXaxis()->SetNdivisions(503);
  grRef->GetXaxis()->SetTimeFormat("%m/%d %H:%M:%S");
  grRef->SetLineColor(kGreen);
  gr0->SetLineColor(kOrange);
  gr1->SetLineColor(kRed);
  gr2->SetLineColor(28);
  gr3->SetLineColor(kBlue);
  gr4->SetLineColor(kMagenta);
  gr5->SetLineColor(kBlack);
  grt->SetLineColor(kBlue);
  grtP->SetLineColor(kRed);

  grRef->GetXaxis()->SetTitle("");
  grRef->SetTitle("");
  grRef->GetYaxis()->SetTitle("pressure(psi)");
  
  //0
    f1->SetLineColor(kOrange);  f1->SetLineStyle(kDashDotted);f1->SetLineWidth(4);
  if(panel0)gr0->Draw("SAME");gr0->Fit(f1);
  double deltap0_new = f1->Eval(tf->Convert()) - f1->Eval(ti->Convert());
  //1
    f1->SetLineColor(kRed);  f1->SetLineStyle(kDashDotted);f1->SetLineWidth(4);
  if(panel1)gr1->Draw("SAME");gr1->Fit(f1);
  double deltap1_new = f1->Eval(tf->Convert()) - f1->Eval(ti->Convert());
  //2
    f1->SetLineColor(28);  f1->SetLineStyle(kDashDotted);f1->SetLineWidth(4);
  if(panel2)gr2->Draw("SAME");gr2->Fit(f1);
  double deltap2_new = f1->Eval(tf->Convert()) - f1->Eval(ti->Convert());
  //3
    f1->SetLineColor(kBlue);  f1->SetLineStyle(kDashDotted);f1->SetLineWidth(4);
  if(panel3)gr3->Draw("SAME");gr3->Fit(f1);
  gPoints->Draw("SAMEP");
  double deltap3_new = f1->Eval(tf->Convert()) - f1->Eval(ti->Convert());
  //4
    f1->SetLineColor(kMagenta);  f1->SetLineStyle(kDashDotted);f1->SetLineWidth(4);
  if(panel4)gr4->Draw("SAME");gr4->Fit(f1); 
  double deltap4_new = f1->Eval(tf->Convert()) - f1->Eval(ti->Convert());
  //5
    f1->SetLineColor(kBlack);  f1->SetLineStyle(kDashDotted);f1->SetLineWidth(4);
  if(panel5)gr5->Draw("SAME");gr5->Fit(f1);
  gPoints->Draw("SAMEP");
  double deltap5_new = f1->Eval(tf->Convert()) - f1->Eval(ti->Convert());

  grt->Draw("SAME");
  grtP->Draw("SAME");

  grRef->GetYaxis()->SetRangeUser(20., 31.);
  grRef->SetTitle(fn.c_str());

  auto legend2 = new TLegend(0.1,0.5,0.48,0.7);
  //legend->SetHeader("The Legend Title","C"); // option "C" allows to center the header
  legend2->AddEntry(grRef,"Referance","l");
  legend2->AddEntry(gr0,"Panel-0","l");
  legend2->AddEntry(gr1,"Panel-1","l");
  legend2->AddEntry(gr2,"Panel-2","l");
  legend2->AddEntry(gr3,"Panel-3","l");
  legend2->AddEntry(gr4,"Panel-4","l");
  legend2->AddEntry(gr5,"Panel-5","l");
  legend2->Draw("SAME");

  //c1->Close();
  
  //Draw temperatures
  auto cT = new TCanvas("cT","cT",1200,900);
  cT->cd();
  grtE->Draw("AL");
  grtE->GetXaxis()->SetTimeDisplay(1);
  grtE->GetXaxis()->SetNdivisions(503);
  grtE->GetXaxis()->SetTimeFormat("%m/%d %H:%M:%S");
  grt->Draw("SAME");
  grtP->Draw("SAME");
  grt->SetLineColor(kGreen);
  grtP->SetLineColor(kBlue);
  grtE->SetLineColor(kRed);

  grtE->GetXaxis()->SetTitle("");
  grtE->SetTitle("");
  grtE->GetYaxis()->SetTitle("temp C");

    auto legendT = new TLegend(0.1,0.5,0.48,0.7);
  //legend->SetHeader("The Legend Title","C"); // option "C" allows to center the header
  legendT->AddEntry(grtE,"Env","l");
  legendT->AddEntry(grtP,"Plane","l");
  legendT->AddEntry(grt,"Referance","l");
  legendT->Draw("SAME");

  //DRAW LEAK RATE
  double pRefLeak = (deltapRef_new - pRefCorr)*(60./deltat)*((1000.)/14.69);
  double pRefLeakAdj = pRefLeak*(3681./1000.);
  double p0Leak = (deltap0_new - p0Corr)*(60./deltat)*(3681./14.69) + pRefLeakAdj;
  double p1Leak = (deltap1_new - p1Corr)*(60./deltat)*(3681./14.69) + pRefLeakAdj;
  double p2Leak = (deltap2_new - p2Corr)*(60./deltat)*(3681./14.69) + pRefLeakAdj;
  double p3Leak = (deltap3_new - p3Corr)*(60./deltat)*(3681./14.69) + pRefLeakAdj;
  double p4Leak = (deltap4_new - p4Corr)*(60./deltat)*(3681./14.69) + pRefLeakAdj;
  double p5Leak = (deltap5_new - p5Corr)*(60./deltat)*(3681./14.69) + pRefLeakAdj;

  std::cout<<"Rref = "<<Rref<<std::endl;
  std::cout<<"Rplane = "<<Rp<<std::endl;
  std::cout<<"Rstraw = "<<RS<<std::endl;
  std::cout<<"Rpanel/Rref "<<Rp/Rref<<std::endl;

  std::cout<<"Temp difference vs pressure ==== "<<std::endl;
  std::cout<<"Referance delta p"<<deltapRef_new<<std::endl;
  std::cout<<"Referance correction = "<< (pRefCorr)<<std::endl;
  std::cout<<"Corrected Referance = "<< (deltapRef_new - pRefCorr)<<std::endl;
  std::cout<<"Temperature panel ="<<tP_i<<"\t"<<tP_f<<std::endl;
  std::cout<<"Temperature Ref ="<<tRef_i<<"\t"<<tRef_f<<std::endl;
  std::cout<<"plane delta T:"<<TfPlane - TiPlane<<std::endl;
  std::cout<<"p0 delta p:"<<deltap0_new<<std::endl;
  std::cout<<"p0 correction"<<p0Corr<<std::endl;
  std::cout<<"p1 delta p:"<<deltap1_new<<std::endl;
  std::cout<<"p1 correction"<<p1Corr<<std::endl;
  std::cout<<"p2 delta p:"<<deltap2_new<<std::endl;
  std::cout<<"p2 correction"<<p2Corr<<std::endl;
  std::cout<<"p3 delta p:"<<deltap3_new<<std::endl;
  std::cout<<"p3 correction"<<p3Corr<<std::endl;
  std::cout<<"p4 delta p:"<<deltap4_new<<std::endl;
  std::cout<<"p5 correction"<<p4Corr<<std::endl;
  std::cout<<"p5 delta p:"<<deltap5_new<<std::endl;
  std::cout<<"p5 correction"<<p5Corr<<std::endl;

  double totalLeak = p0Leak + p1Leak + p2Leak + p3Leak + p4Leak + p5Leak;

  std::cout<<"Ref leak rate = "<<pRefLeak<<" sccm -> "<<100.*pRefLeak/0.014<<"% of per panel budget"<<std::endl;
  std::cout<<"p0 leak rate = "<<p0Leak<<" sccm -> "<<100.*(p0Leak)/0.014<<"% of per panel budget"<<std::endl;
  std::cout<<"p1 leak rate = "<<p1Leak<<" sccm -> "<<100.*(p1Leak)/0.014<<"% of per panel budget"<<std::endl;
  std::cout<<"p2 leak rate = "<<p2Leak<<" sccm -> "<<100.*(p2Leak)/0.014<<"% of per panel budget"<<std::endl;
  std::cout<<"p3 leak rate = "<<p3Leak<<" sccm -> "<<100.*(p3Leak)/0.014<<"% of per panel budget"<<std::endl;
  std::cout<<"p4 leak rate = "<<p4Leak<<" sccm -> "<<100.*(p4Leak)/0.014<<"% of per panel budget"<<std::endl;
  std::cout<<"p5 leak rate = "<<p5Leak<<" sccm -> "<<100.*(p5Leak)/0.014<<"% of per panel budget"<<std::endl;
  std::cout<<"sum = "<<totalLeak<<std::endl;
  std::cout<<"plane leak rate = "<<totalLeak<<" sccm -->"<<100.*(totalLeak)/(0.014*6.)<<"% of per plane budget"<<std::endl;

  std::vector<double> leak_rates = {-p0Leak,-p1Leak,-p2Leak,-p3Leak,-p4Leak,-p5Leak};
  std::vector<double> index = {0.,1.,2.,3.,4.,5.};

  auto c2 = new TCanvas("c2","c2",1200,900);
  c2->cd();
  auto leak = new TGraph(index.size(),&index[0],&leak_rates[0]);
  leak->Draw("AP");
  leak->SetMarkerStyle(kOpenSquare);
  leak->SetMarkerColor(kBlue);
  leak->SetMarkerSize(2);
  leak->SetTitle(split_name.at(0).c_str());
  leak->GetYaxis()->SetTitle("leak rate (sccm)");
  leak->GetXaxis()->SetTitle("panel number");
  leak->GetYaxis()->SetRangeUser(-0.02, 0.1);
  TLine *panel_budget = new TLine(0.5,0.014,5.5,0.014);
  panel_budget->SetLineColor(kRed);
  panel_budget->SetLineWidth(4);
  panel_budget->SetLineStyle(kDashDotted);
  panel_budget->Draw("SAME");
  TLine *plane_budget = new TLine(0.5,0.014*6,5.5,0.014*6);
  plane_budget->SetLineColor(kRed+2);
  plane_budget->SetLineWidth(4);
  plane_budget->SetLineStyle(kDashDotted);
  plane_budget->Draw("SAME");
  TLine *plane_leak = new TLine(0.5,std::abs(totalLeak),5.5,std::abs(totalLeak));
  plane_leak->SetLineColor(kBlue);
  plane_leak->SetLineWidth(4);
  plane_leak->SetLineStyle(kSolid);
  plane_leak->Draw("SAME");


  auto legend = new TLegend(0.1,0.5,0.48,0.7);
  //legend->SetHeader("The Legend Title","C"); // option "C" allows to center the header
  std::string plane_legend = "Plane leak rate = " + std::to_string(std::abs(totalLeak)) + " sccm";
  legend->AddEntry(leak,"Panel leak rates","p");
  legend->AddEntry(plane_leak,plane_legend.c_str(),"l");
  legend->AddEntry(panel_budget,"Panel leak budget","l");
  legend->AddEntry(plane_budget,"Plane leak budget","l");
  legend->Draw("SAME");

  //c2->Close();

}

std::vector<std::string> split(std::string s, char d){
  
  std::vector<std::string> split_string;
  std::stringstream ss(s);
  std::string token;
  while (std::getline(ss, token, d)) {
    split_string.push_back(token);
  }

  return split_string;
}


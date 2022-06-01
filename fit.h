TF1 *fitFcn;

Double_t fitFunction(Double_t *x, Double_t *par){
  
  Float_t xx =x[0];
  return par[0]*xx + par[1];

}


bool DoFit(const char* fitter, TVirtualPad *pad, Int_t npass, TGraph *g) {
	
	gRandom = new TRandom3();
	ROOT::Math::MinimizerOptions::SetDefaultMinimizer(fitter);
	pad->SetGrid();
	fitFcn->Update();
	std::string title = std::string(fitter) + " fit bench";

	TString fitterType(fitter);

	bool ok = true;

	for (Int_t pass=0;pass<npass;pass++)
	{
		if (pass%100 == 0) printf("pass : %d\n",pass);
		else printf(".");
		if (pass == 0){
			fitFcn->SetParameter(0,0.02);
			fitFcn->SetParameter(1,28.);
		}
		int iret = g->Fit(fitFcn,"RM");
		if (iret!=0) Error("DoFit","Fit pass %d failed !",pass);
		cout<<"-------Chi square-----------"<<fitFcn->GetChisquare()<<endl; 
	}

	g->Fit(fitFcn);
	fitFcn->Draw("same");
	return true;
}


int fit(TGraph *gr){

  
  std::cout<<"fit function lol"<<std::endl;
  TH1::AddDirectory(kFALSE);

  fitFcn = new TF1("fitFcn",fitFunction,0.1,900,2);
  fitFcn->SetDrawOption("P");
  fitFcn->SetLineColor(kRed);
	
  gStyle->SetOptFit();
  auto npass = 10;

  DoFit("Minuit2",gPad,npass,gr);
  return 0;
  
}

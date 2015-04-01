program DcaEncTest;

uses
  Forms,
  DcaEncTestForm in 'DcaEncTestForm.pas' {Form1},
  DcaEncAPI in 'DcaEncAPI.pas';

{$R *.res}

begin
  Application.Initialize;
  Application.Title := 'DCA Enc';
  Application.CreateForm(TForm1, Form1);
  Application.Run;
end.

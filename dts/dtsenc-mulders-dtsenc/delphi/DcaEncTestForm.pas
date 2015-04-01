unit DcaEncTestForm;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, StdCtrls, DcaEncAPI, ComCtrls;

type
  TForm1 = class(TForm)
    Button1: TButton;
    ProgressBar1: TProgressBar;
    procedure Button1Click(Sender: TObject);
    procedure FormCloseQuery(Sender: TObject; var CanClose: Boolean);
  private
    { Private-Deklarationen }
  public
    { Public-Deklarationen }
  end;

var
  Form1: TForm1;

implementation

{$R *.dfm}

procedure TForm1.Button1Click(Sender: TObject);
var
  Context: TDcaEncContext;
  InputBuffer: array of LongInt;
  OutputBuffer: array of Byte;
  SamplesInInput: Integer;
  MaxBytesInOutput: Integer;
  BytesReturned: Cardinal;
  BytesWritten: Cardinal;
  i: Integer;
  h: THandle;
const
  ChannelsInInput: Integer = 2;
  EncodeSteps: Integer = 10240;
begin
  Button1.Enabled := False;
  ProgressBar1.Position := 0;
  Application.ProcessMessages;

  //Create DCA Enc context
  Context := dcaenc_create(44100, DCAENC_CHANNELS_STEREO, 768000, DCAENC_FLAG_BIGENDIAN);

  //Context created successfully?
  if(Context = nil) then
  begin
    ShowMessage('Failed to create context!');
    Exit;
  end;

  //Detect input/output size
  SamplesInInput := dcaenc_input_size(Context);
  MaxBytesInOutput := dcaenc_output_size(Context);

  //Some feedback
  ShowMessage('SamplesInInput = ' + IntToStr(SamplesInInput));
  ShowMessage('MaxBytesInOutput = ' + IntToStr(MaxBytesInOutput));

  //Allocate buffers
  SetLength(InputBuffer, SamplesInInput * ChannelsInInput);
  SetLength(OutputBuffer, MaxBytesInOutput);

  //ZeroBuffers
  ZeroMemory(@InputBuffer[0], SizeOf(LongInt) * SamplesInInput * ChannelsInInput);
  ZeroMemory(@OutputBuffer[0], SizeOf(Byte) * MaxBytesInOutput);

  //Create an output file
  h := CreateFile('Test.dts', GENERIC_WRITE, FILE_SHARE_READ, nil, CREATE_ALWAYS, 0, 0);
  if(h = INVALID_HANDLE_VALUE) then
  begin
    ShowMessage('Failed to create output file!');
    Exit;
  end;

  //Encode loop
  for i := 0 to EncodeSteps do
  begin
    // TODO: Load the next 'SamplesInInput' samples into 'InputBuffer' here!
    //       Be aware that samples have to be 32-Bit Signed for DCAEnc.

    BytesReturned := dcaenc_convert_s32(Context, @InputBuffer[0], @OutputBuffer[0]);
    WriteFile(h, OutputBuffer[0], BytesReturned, BytesWritten, nil);
    ProgressBar1.Position := Round((i / EncodeSteps) * 100.0);
    Application.ProcessMessages;
  end;

  //Finalize Encode
  BytesReturned := dcaenc_destroy(Context, @OutputBuffer[0]);
  WriteFile(h, OutputBuffer[0], BytesReturned, BytesWritten, nil);

  //Close output
  CloseHandle(h);

  //We are done!
  ShowMessage('Encode has completed :-)');
  Button1.Enabled := True;
  Application.ProcessMessages;
end;

procedure TForm1.FormCloseQuery(Sender: TObject; var CanClose: Boolean);
begin
  CanClose := Button1.Enabled;
end;

end.

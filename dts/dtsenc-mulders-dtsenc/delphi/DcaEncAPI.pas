unit DcaEncAPI;

interface

uses Windows, SysUtils;

const
  DCAENC_FLAG_28BIT = 1;
  DCAENC_FLAG_BIGENDIAN = 2;
  DCAENC_FLAG_LFE = 4;
  DCAENC_FLAG_PERFECT_QMF = 8;
  DCAENC_FLAG_IEC_WRAP = 16;

  DCAENC_CHANNELS_MONO = 0;
  DCAENC_CHANNELS_DUAL_MONO = 1;
  DCAENC_CHANNELS_STEREO = 2;
  DCAENC_CHANNELS_STEREO_SUMDIFF = 3;
  DCAENC_CHANNELS_STEREO_TOTAL = 4;
  DCAENC_CHANNELS_3FRONT = 5;
  DCAENC_CHANNELS_2FRONT_1REAR = 6;
  DCAENC_CHANNELS_3FRONT_1REAR = 7;
  DCAENC_CHANNELS_2FRONT_2REAR = 8;
  DCAENC_CHANNELS_3FRONT_2REAR = 9;
  DCAENC_CHANNELS_4FRONT_2REAR = 10;
  DCAENC_CHANNELS_3FRONT_2REAR_1OV = 11;
  DCAENC_CHANNELS_3FRONT_3REAR = 12;
  DCAENC_CHANNELS_5FRONT_2REAR = 13;
  DCAENC_CHANNELS_4FRONT_4REAR = 14;
  DCAENC_CHANNELS_5FRONT_3REAR = 15;

type
  TDcaEncContext = Pointer;

function dcaenc_create(sample_rate, channel_config, approx_bitrate, flags: integer): TDcaEncContext; cdecl; external 'dcaenc-0.dll';
function dcaenc_bitrate(c : TDcaEncContext): integer; cdecl; external 'dcaenc-0.dll';
function dcaenc_input_size(c : TDcaEncContext): integer; cdecl; external 'dcaenc-0.dll';
function dcaenc_output_size(c : TDcaEncContext): integer; cdecl; external 'dcaenc-0.dll';
function dcaenc_convert_s32(c : TDcaEncContext; input: PLongInt; output: PByte): integer; cdecl; external 'dcaenc-0.dll';
function dcaenc_destroy(c : TDcaEncContext; output: PByte): integer; cdecl; external 'dcaenc-0.dll';

implementation

end.


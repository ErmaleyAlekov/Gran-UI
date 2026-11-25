#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct type_ConstantBuffer_FrameConstants
{
    float2 viewport;
    float2 inverseViewport;
    uint instanceBase;
    uint clipCount;
    uint textureCount;
    uint rampRows;
    uint atlasSlot;
    uint rampSlot;
    uint2 reserved;
};

struct GpuInstance
{
    float4 rect;
    float4 radii;
    float4 border;
    float4 transform;
    float2 translate;
    float opacity;
    float param;
    uint kind;
    uint clip;
    uint fill;
    uint stroke;
    float4 extra;
    uint4 paint;
};

struct type_StructuredBuffer_GpuInstance
{
    GpuInstance _m0[1];
};

struct GpuClip
{
    float4 rect;
    float4 radii;
    float4 _inverse;
    float2 inverseTranslate;
    float2 padding;
};

struct type_StructuredBuffer_GpuClip
{
    GpuClip _m0[1];
};

struct psMain_out
{
    float4 out_var_SV_Target [[color(0)]];
};

struct psMain_in
{
    float2 in_var_LOCAL [[user(locn0)]];
    uint in_var_INSTANCE [[user(locn1)]];
};

fragment psMain_out psMain(psMain_in in [[stage_in]], constant type_ConstantBuffer_FrameConstants& frame [[buffer(0)]], const device type_StructuredBuffer_GpuInstance& instances [[buffer(1)]], const device type_StructuredBuffer_GpuClip& clips [[buffer(2)]], texture2d<float> atlasTexture [[texture(0)]], texture2d<float> rampTexture [[texture(1)]], texture2d<float> imageTexture [[texture(2)]], texture2d<float> backdropTexture [[texture(3)]], sampler linearSampler [[sampler(0)]], float4 gl_FragCoord [[position]])
{
    psMain_out out = {};
    float4 _859;
    if (instances._m0[in.in_var_INSTANCE].kind == 0u)
    {
        float2 _136 = instances._m0[in.in_var_INSTANCE].rect.zw * 0.5;
        float2 _139 = (in.in_var_LOCAL - instances._m0[in.in_var_INSTANCE].rect.xy) - _136;
        float _158;
        do
        {
            if (_139.x < 0.0)
            {
                if (_139.y < 0.0)
                {
                    _158 = instances._m0[in.in_var_INSTANCE].radii.x;
                    break;
                }
                _158 = instances._m0[in.in_var_INSTANCE].radii.w;
                break;
            }
            if (_139.y < 0.0)
            {
                _158 = instances._m0[in.in_var_INSTANCE].radii.y;
                break;
            }
            _158 = instances._m0[in.in_var_INSTANCE].radii.z;
            break;
        } while(false);
        float2 _162 = (abs(_139) - _136) + float2(_158);
        float _170 = (precise::min(precise::max(_162.x, _162.y), 0.0) + length(precise::max(_162, float2(0.0)))) - _158;
        float _171 = fwidth(_170);
        float _175 = fast::clamp(0.5 - (_170 / precise::max(_171, 9.9999997473787516355514526367188e-05)), 0.0, 1.0);
        float _261;
        if (any(instances._m0[in.in_var_INSTANCE].border > float4(0.0)))
        {
            float4 _201 = float4(instances._m0[in.in_var_INSTANCE].rect.xy + instances._m0[in.in_var_INSTANCE].border.wx, (instances._m0[in.in_var_INSTANCE].rect.zw - instances._m0[in.in_var_INSTANCE].border.wx) - instances._m0[in.in_var_INSTANCE].border.yz);
            float4 _212 = precise::max(instances._m0[in.in_var_INSTANCE].radii - float4(precise::max(instances._m0[in.in_var_INSTANCE].border.w, instances._m0[in.in_var_INSTANCE].border.x), precise::max(instances._m0[in.in_var_INSTANCE].border.y, instances._m0[in.in_var_INSTANCE].border.x), precise::max(instances._m0[in.in_var_INSTANCE].border.y, instances._m0[in.in_var_INSTANCE].border.z), precise::max(instances._m0[in.in_var_INSTANCE].border.w, instances._m0[in.in_var_INSTANCE].border.z)), float4(0.0));
            float2 _213 = _201.zw;
            float _260;
            if (all(_213 > float2(0.0)))
            {
                float2 _219 = _213 * 0.5;
                float2 _222 = (in.in_var_LOCAL - _201.xy) - _219;
                float _241;
                do
                {
                    if (_222.x < 0.0)
                    {
                        if (_222.y < 0.0)
                        {
                            _241 = _212.x;
                            break;
                        }
                        _241 = _212.w;
                        break;
                    }
                    if (_222.y < 0.0)
                    {
                        _241 = _212.y;
                        break;
                    }
                    _241 = _212.z;
                    break;
                } while(false);
                float2 _245 = (abs(_222) - _219) + float2(_241);
                float _253 = (precise::min(precise::max(_245.x, _245.y), 0.0) + length(precise::max(_245, float2(0.0)))) - _241;
                _260 = precise::min(_175, fast::clamp(0.5 - (_253 / precise::max(fwidth(_253), 9.9999997473787516355514526367188e-05)), 0.0, 1.0));
            }
            else
            {
                _260 = 0.0;
            }
            _261 = _260;
        }
        else
        {
            _261 = _175;
        }
        float4 _317;
        if (instances._m0[in.in_var_INSTANCE].paint.z != 0u)
        {
            float _299;
            if (instances._m0[in.in_var_INSTANCE].paint.z == 1u)
            {
                float2 _272 = instances._m0[in.in_var_INSTANCE].extra.zw - instances._m0[in.in_var_INSTANCE].extra.xy;
                _299 = dot(in.in_var_LOCAL - instances._m0[in.in_var_INSTANCE].extra.xy, _272) / precise::max(dot(_272, _272), 9.9999999747524270787835121154785e-07);
            }
            else
            {
                float _298;
                if (instances._m0[in.in_var_INSTANCE].paint.z == 2u)
                {
                    _298 = length((in.in_var_LOCAL - instances._m0[in.in_var_INSTANCE].extra.xy) / precise::max(instances._m0[in.in_var_INSTANCE].extra.zw, float2(0.001000000047497451305389404296875)));
                }
                else
                {
                    float2 _289 = in.in_var_LOCAL - instances._m0[in.in_var_INSTANCE].extra.xy;
                    _298 = fract((precise::atan2(_289.x, -_289.y) - instances._m0[in.in_var_INSTANCE].extra.z) * 0.15915493667125701904296875);
                }
                _299 = _298;
            }
            _317 = rampTexture.sample(linearSampler, float2(((fast::clamp(_299, 0.0, 1.0) * 255.0) + 0.5) * 0.00390625, (float(instances._m0[in.in_var_INSTANCE].paint.y) + 0.5) / float(max(frame.rampRows, 1u))), level(0.0));
        }
        else
        {
            _317 = float4(float(instances._m0[in.in_var_INSTANCE].fill & 255u), float((instances._m0[in.in_var_INSTANCE].fill >> 8u) & 255u), float((instances._m0[in.in_var_INSTANCE].fill >> 16u) & 255u), float(instances._m0[in.in_var_INSTANCE].fill >> 24u)) * float4(0.0039215688593685626983642578125);
        }
        _859 = (_317 * _261) + ((float4(float(instances._m0[in.in_var_INSTANCE].stroke & 255u), float((instances._m0[in.in_var_INSTANCE].stroke >> 8u) & 255u), float((instances._m0[in.in_var_INSTANCE].stroke >> 16u) & 255u), float(instances._m0[in.in_var_INSTANCE].stroke >> 24u)) * float4(0.0039215688593685626983642578125)) * fast::clamp(_175 - _261, 0.0, 1.0));
    }
    else
    {
        float4 _858;
        if (instances._m0[in.in_var_INSTANCE].kind == 1u)
        {
            float2 _340 = instances._m0[in.in_var_INSTANCE].rect.zw * 0.5;
            float2 _343 = (in.in_var_LOCAL - instances._m0[in.in_var_INSTANCE].rect.xy) - _340;
            float _362;
            do
            {
                if (_343.x < 0.0)
                {
                    if (_343.y < 0.0)
                    {
                        _362 = instances._m0[in.in_var_INSTANCE].radii.x;
                        break;
                    }
                    _362 = instances._m0[in.in_var_INSTANCE].radii.w;
                    break;
                }
                if (_343.y < 0.0)
                {
                    _362 = instances._m0[in.in_var_INSTANCE].radii.y;
                    break;
                }
                _362 = instances._m0[in.in_var_INSTANCE].radii.z;
                break;
            } while(false);
            float2 _366 = (abs(_343) - _340) + float2(_362);
            float _374 = (precise::min(precise::max(_366.x, _366.y), 0.0) + length(precise::max(_366, float2(0.0)))) - _362;
            float _375 = fwidth(_374);
            float _516;
            do
            {
                float _404;
                float2 _397 = precise::max(_340 + float2(instances._m0[in.in_var_INSTANCE].extra.z), float2(0.0));
                float2 _401 = in.in_var_LOCAL - ((instances._m0[in.in_var_INSTANCE].rect.xy + _340) + instances._m0[in.in_var_INSTANCE].extra.xy);
                float _420;
                do
                {
                    _404 = _401.x;
                    if (_404 < 0.0)
                    {
                        if (_401.y < 0.0)
                        {
                            _420 = instances._m0[in.in_var_INSTANCE].radii.x;
                            break;
                        }
                        _420 = instances._m0[in.in_var_INSTANCE].radii.w;
                        break;
                    }
                    if (_401.y < 0.0)
                    {
                        _420 = instances._m0[in.in_var_INSTANCE].radii.y;
                        break;
                    }
                    _420 = instances._m0[in.in_var_INSTANCE].radii.z;
                    break;
                } while(false);
                float _423 = _397.x;
                float _424 = _397.y;
                float _426 = precise::min(precise::max(_420 + instances._m0[in.in_var_INSTANCE].extra.z, 0.0), precise::min(_423, _424));
                float2 _430 = (abs(_401) - _397) + float2(_426);
                float _438 = (precise::min(precise::max(_430.x, _430.y), 0.0) + length(precise::max(_430, float2(0.0)))) - _426;
                float _439 = fwidth(_438);
                if (instances._m0[in.in_var_INSTANCE].param < 0.5)
                {
                    _516 = fast::clamp(0.5 - (_438 / precise::max(_439, 9.9999997473787516355514526367188e-05)), 0.0, 1.0);
                    break;
                }
                float _447 = _401.y;
                float _448 = _447 - _424;
                float _449 = _447 + _424;
                float _451 = fast::clamp((-3.0) * instances._m0[in.in_var_INSTANCE].param, _448, _449);
                float _454 = fast::clamp(3.0 * instances._m0[in.in_var_INSTANCE].param, _448, _449) - _451;
                float _455 = _454 * 0.25;
                float _457 = _451 + (_454 * 0.125);
                float _462;
                _462 = 0.0;
                float _459 = _457;
                int _464 = 0;
                for (; _464 < 4; )
                {
                    float _472 = precise::min((_424 - _426) - abs(_447 - _459), 0.0);
                    float _479 = (_423 - _426) + sqrt(precise::max(0.0, (_426 * _426) - (_472 * _472)));
                    float2 _485 = (float2(_404) + float2(-_479, _479)) * (0.707106769084930419921875 / instances._m0[in.in_var_INSTANCE].param);
                    float2 _488 = float2(int2(sign(_485)));
                    float2 _489 = abs(_485);
                    float2 _496 = float2(1.0) + ((float2(0.2783930003643035888671875) + ((float2(0.23038899898529052734375) + ((_489 * _489) * 0.07810799777507781982421875)) * _489)) * _489);
                    float2 _497 = _496 * _496;
                    float2 _502 = float2(0.5) + ((_488 - (_488 / (_497 * _497))) * 0.5);
                    float _506 = _459 * _459;
                    _459 += _455;
                    _462 += (((_502.y - _502.x) * (exp((-_506) / ((2.0 * instances._m0[in.in_var_INSTANCE].param) * instances._m0[in.in_var_INSTANCE].param)) / (2.5066282749176025390625 * instances._m0[in.in_var_INSTANCE].param))) * _455);
                    _464++;
                    continue;
                }
                _516 = _462;
                break;
            } while(false);
            _858 = ((float4(float(instances._m0[in.in_var_INSTANCE].fill & 255u), float((instances._m0[in.in_var_INSTANCE].fill >> 8u) & 255u), float((instances._m0[in.in_var_INSTANCE].fill >> 16u) & 255u), float(instances._m0[in.in_var_INSTANCE].fill >> 24u)) * float4(0.0039215688593685626983642578125)) * _516) * (1.0 - fast::clamp(0.5 - (_374 / precise::max(_375, 9.9999997473787516355514526367188e-05)), 0.0, 1.0));
        }
        else
        {
            float4 _857;
            if (instances._m0[in.in_var_INSTANCE].kind == 2u)
            {
                float2 _525 = instances._m0[in.in_var_INSTANCE].rect.zw * 0.5;
                float2 _528 = (in.in_var_LOCAL - instances._m0[in.in_var_INSTANCE].rect.xy) - _525;
                float _547;
                do
                {
                    if (_528.x < 0.0)
                    {
                        if (_528.y < 0.0)
                        {
                            _547 = instances._m0[in.in_var_INSTANCE].radii.x;
                            break;
                        }
                        _547 = instances._m0[in.in_var_INSTANCE].radii.w;
                        break;
                    }
                    if (_528.y < 0.0)
                    {
                        _547 = instances._m0[in.in_var_INSTANCE].radii.y;
                        break;
                    }
                    _547 = instances._m0[in.in_var_INSTANCE].radii.z;
                    break;
                } while(false);
                float2 _551 = (abs(_528) - _525) + float2(_547);
                float _559 = (precise::min(precise::max(_551.x, _551.y), 0.0) + length(precise::max(_551, float2(0.0)))) - _547;
                float _560 = fwidth(_559);
                float _702;
                do
                {
                    float _590;
                    float _580 = instances._m0[in.in_var_INSTANCE].extra.z * (-1.0);
                    float2 _583 = precise::max(_525 + float2(_580), float2(0.0));
                    float2 _587 = in.in_var_LOCAL - ((instances._m0[in.in_var_INSTANCE].rect.xy + _525) + instances._m0[in.in_var_INSTANCE].extra.xy);
                    float _606;
                    do
                    {
                        _590 = _587.x;
                        if (_590 < 0.0)
                        {
                            if (_587.y < 0.0)
                            {
                                _606 = instances._m0[in.in_var_INSTANCE].radii.x;
                                break;
                            }
                            _606 = instances._m0[in.in_var_INSTANCE].radii.w;
                            break;
                        }
                        if (_587.y < 0.0)
                        {
                            _606 = instances._m0[in.in_var_INSTANCE].radii.y;
                            break;
                        }
                        _606 = instances._m0[in.in_var_INSTANCE].radii.z;
                        break;
                    } while(false);
                    float _609 = _583.x;
                    float _610 = _583.y;
                    float _612 = precise::min(precise::max(_606 + _580, 0.0), precise::min(_609, _610));
                    float2 _616 = (abs(_587) - _583) + float2(_612);
                    float _624 = (precise::min(precise::max(_616.x, _616.y), 0.0) + length(precise::max(_616, float2(0.0)))) - _612;
                    float _625 = fwidth(_624);
                    if (instances._m0[in.in_var_INSTANCE].param < 0.5)
                    {
                        _702 = fast::clamp(0.5 - (_624 / precise::max(_625, 9.9999997473787516355514526367188e-05)), 0.0, 1.0);
                        break;
                    }
                    float _633 = _587.y;
                    float _634 = _633 - _610;
                    float _635 = _633 + _610;
                    float _637 = fast::clamp((-3.0) * instances._m0[in.in_var_INSTANCE].param, _634, _635);
                    float _640 = fast::clamp(3.0 * instances._m0[in.in_var_INSTANCE].param, _634, _635) - _637;
                    float _641 = _640 * 0.25;
                    float _643 = _637 + (_640 * 0.125);
                    float _648;
                    _648 = 0.0;
                    float _645 = _643;
                    int _650 = 0;
                    for (; _650 < 4; )
                    {
                        float _658 = precise::min((_610 - _612) - abs(_633 - _645), 0.0);
                        float _665 = (_609 - _612) + sqrt(precise::max(0.0, (_612 * _612) - (_658 * _658)));
                        float2 _671 = (float2(_590) + float2(-_665, _665)) * (0.707106769084930419921875 / instances._m0[in.in_var_INSTANCE].param);
                        float2 _674 = float2(int2(sign(_671)));
                        float2 _675 = abs(_671);
                        float2 _682 = float2(1.0) + ((float2(0.2783930003643035888671875) + ((float2(0.23038899898529052734375) + ((_675 * _675) * 0.07810799777507781982421875)) * _675)) * _675);
                        float2 _683 = _682 * _682;
                        float2 _688 = float2(0.5) + ((_674 - (_674 / (_683 * _683))) * 0.5);
                        float _692 = _645 * _645;
                        _645 += _641;
                        _648 += (((_688.y - _688.x) * (exp((-_692) / ((2.0 * instances._m0[in.in_var_INSTANCE].param) * instances._m0[in.in_var_INSTANCE].param)) / (2.5066282749176025390625 * instances._m0[in.in_var_INSTANCE].param))) * _641);
                        _650++;
                        continue;
                    }
                    _702 = _648;
                    break;
                } while(false);
                _857 = ((float4(float(instances._m0[in.in_var_INSTANCE].fill & 255u), float((instances._m0[in.in_var_INSTANCE].fill >> 8u) & 255u), float((instances._m0[in.in_var_INSTANCE].fill >> 16u) & 255u), float(instances._m0[in.in_var_INSTANCE].fill >> 24u)) * float4(0.0039215688593685626983642578125)) * (1.0 - _702)) * fast::clamp(0.5 - (_559 / precise::max(_560, 9.9999997473787516355514526367188e-05)), 0.0, 1.0);
            }
            else
            {
                float4 _856;
                if (instances._m0[in.in_var_INSTANCE].kind == 3u)
                {
                    _856 = (float4(float(instances._m0[in.in_var_INSTANCE].fill & 255u), float((instances._m0[in.in_var_INSTANCE].fill >> 8u) & 255u), float((instances._m0[in.in_var_INSTANCE].fill >> 16u) & 255u), float(instances._m0[in.in_var_INSTANCE].fill >> 24u)) * float4(0.0039215688593685626983642578125)) * atlasTexture.sample(linearSampler, mix(instances._m0[in.in_var_INSTANCE].extra.xy, instances._m0[in.in_var_INSTANCE].extra.zw, (in.in_var_LOCAL - instances._m0[in.in_var_INSTANCE].rect.xy) / instances._m0[in.in_var_INSTANCE].rect.zw), level(0.0)).x;
                }
                else
                {
                    float4 _855;
                    if (instances._m0[in.in_var_INSTANCE].kind == 4u)
                    {
                        float2 _742 = in.in_var_LOCAL - instances._m0[in.in_var_INSTANCE].rect.xy;
                        float2 _750 = instances._m0[in.in_var_INSTANCE].rect.zw * 0.5;
                        float2 _751 = _742 - _750;
                        float _770;
                        do
                        {
                            if (_751.x < 0.0)
                            {
                                if (_751.y < 0.0)
                                {
                                    _770 = instances._m0[in.in_var_INSTANCE].radii.x;
                                    break;
                                }
                                _770 = instances._m0[in.in_var_INSTANCE].radii.w;
                                break;
                            }
                            if (_751.y < 0.0)
                            {
                                _770 = instances._m0[in.in_var_INSTANCE].radii.y;
                                break;
                            }
                            _770 = instances._m0[in.in_var_INSTANCE].radii.z;
                            break;
                        } while(false);
                        float2 _774 = (abs(_751) - _750) + float2(_770);
                        float _782 = (precise::min(precise::max(_774.x, _774.y), 0.0) + length(precise::max(_774, float2(0.0)))) - _770;
                        _855 = imageTexture.sample(linearSampler, mix(instances._m0[in.in_var_INSTANCE].extra.xy, instances._m0[in.in_var_INSTANCE].extra.zw, _742 / instances._m0[in.in_var_INSTANCE].rect.zw), level(0.0)) * fast::clamp(0.5 - (_782 / precise::max(fwidth(_782), 9.9999997473787516355514526367188e-05)), 0.0, 1.0);
                    }
                    else
                    {
                        float4 _808 = float4(float(instances._m0[in.in_var_INSTANCE].fill & 255u), float((instances._m0[in.in_var_INSTANCE].fill >> 8u) & 255u), float((instances._m0[in.in_var_INSTANCE].fill >> 16u) & 255u), float(instances._m0[in.in_var_INSTANCE].fill >> 24u)) * float4(0.0039215688593685626983642578125);
                        float2 _810 = instances._m0[in.in_var_INSTANCE].rect.zw * 0.5;
                        float2 _813 = (in.in_var_LOCAL - instances._m0[in.in_var_INSTANCE].rect.xy) - _810;
                        float _832;
                        do
                        {
                            if (_813.x < 0.0)
                            {
                                if (_813.y < 0.0)
                                {
                                    _832 = instances._m0[in.in_var_INSTANCE].radii.x;
                                    break;
                                }
                                _832 = instances._m0[in.in_var_INSTANCE].radii.w;
                                break;
                            }
                            if (_813.y < 0.0)
                            {
                                _832 = instances._m0[in.in_var_INSTANCE].radii.y;
                                break;
                            }
                            _832 = instances._m0[in.in_var_INSTANCE].radii.z;
                            break;
                        } while(false);
                        float2 _836 = (abs(_813) - _810) + float2(_832);
                        float _844 = (precise::min(precise::max(_836.x, _836.y), 0.0) + length(precise::max(_836, float2(0.0)))) - _832;
                        _855 = (_808 + (backdropTexture.sample(linearSampler, (gl_FragCoord.xy * frame.inverseViewport), level(0.0)) * (1.0 - _808.w))) * fast::clamp(0.5 - (_844 / precise::max(fwidth(_844), 9.9999997473787516355514526367188e-05)), 0.0, 1.0);
                    }
                    _856 = _855;
                }
                _857 = _856;
            }
            _858 = _857;
        }
        _859 = _858;
    }
    float _935;
    do
    {
        if (instances._m0[in.in_var_INSTANCE].clip == 0u)
        {
            _935 = 1.0;
            break;
        }
        uint _870 = min(instances._m0[in.in_var_INSTANCE].clip, (max(frame.clipCount, 1u) - 1u));
        float2 _895 = clips._m0[_870].rect.zw * 0.5;
        float2 _898 = ((float2((clips._m0[_870]._inverse.x * gl_FragCoord.x) + (clips._m0[_870]._inverse.z * gl_FragCoord.y), (clips._m0[_870]._inverse.y * gl_FragCoord.x) + (clips._m0[_870]._inverse.w * gl_FragCoord.y)) + clips._m0[_870].inverseTranslate) - clips._m0[_870].rect.xy) - _895;
        float _917;
        do
        {
            if (_898.x < 0.0)
            {
                if (_898.y < 0.0)
                {
                    _917 = clips._m0[_870].radii.x;
                    break;
                }
                _917 = clips._m0[_870].radii.w;
                break;
            }
            if (_898.y < 0.0)
            {
                _917 = clips._m0[_870].radii.y;
                break;
            }
            _917 = clips._m0[_870].radii.z;
            break;
        } while(false);
        float2 _921 = (abs(_898) - _895) + float2(_917);
        float _929 = (precise::min(precise::max(_921.x, _921.y), 0.0) + length(precise::max(_921, float2(0.0)))) - _917;
        _935 = fast::clamp(0.5 - (_929 / precise::max(fwidth(_929), 9.9999997473787516355514526367188e-05)), 0.0, 1.0);
        break;
    } while(false);
    out.out_var_SV_Target = (_859 * instances._m0[in.in_var_INSTANCE].opacity) * _935;
    return out;
}


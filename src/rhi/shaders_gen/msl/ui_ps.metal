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
    float4 _852;
    if (instances._m0[in.in_var_INSTANCE].kind == 0u)
    {
        float2 _135 = instances._m0[in.in_var_INSTANCE].rect.zw * 0.5;
        float2 _138 = (in.in_var_LOCAL - instances._m0[in.in_var_INSTANCE].rect.xy) - _135;
        float _157;
        do
        {
            if (_138.x < 0.0)
            {
                if (_138.y < 0.0)
                {
                    _157 = instances._m0[in.in_var_INSTANCE].radii.x;
                    break;
                }
                _157 = instances._m0[in.in_var_INSTANCE].radii.w;
                break;
            }
            if (_138.y < 0.0)
            {
                _157 = instances._m0[in.in_var_INSTANCE].radii.y;
                break;
            }
            _157 = instances._m0[in.in_var_INSTANCE].radii.z;
            break;
        } while(false);
        float2 _161 = (abs(_138) - _135) + float2(_157);
        float _169 = (precise::min(precise::max(_161.x, _161.y), 0.0) + length(precise::max(_161, float2(0.0)))) - _157;
        float _170 = fwidth(_169);
        float _174 = fast::clamp(0.5 - (_169 / precise::max(_170, 9.9999997473787516355514526367188e-05)), 0.0, 1.0);
        float _254;
        if (any(instances._m0[in.in_var_INSTANCE].border > float4(0.0)))
        {
            float4 _200 = float4(instances._m0[in.in_var_INSTANCE].rect.xy + instances._m0[in.in_var_INSTANCE].border.wx, (instances._m0[in.in_var_INSTANCE].rect.zw - instances._m0[in.in_var_INSTANCE].border.wx) - instances._m0[in.in_var_INSTANCE].border.yz);
            float4 _211 = precise::max(instances._m0[in.in_var_INSTANCE].radii - float4(precise::max(instances._m0[in.in_var_INSTANCE].border.w, instances._m0[in.in_var_INSTANCE].border.x), precise::max(instances._m0[in.in_var_INSTANCE].border.y, instances._m0[in.in_var_INSTANCE].border.x), precise::max(instances._m0[in.in_var_INSTANCE].border.y, instances._m0[in.in_var_INSTANCE].border.z), precise::max(instances._m0[in.in_var_INSTANCE].border.w, instances._m0[in.in_var_INSTANCE].border.z)), float4(0.0));
            float2 _213 = _200.zw * 0.5;
            float2 _216 = (in.in_var_LOCAL - _200.xy) - _213;
            float _235;
            do
            {
                if (_216.x < 0.0)
                {
                    if (_216.y < 0.0)
                    {
                        _235 = _211.x;
                        break;
                    }
                    _235 = _211.w;
                    break;
                }
                if (_216.y < 0.0)
                {
                    _235 = _211.y;
                    break;
                }
                _235 = _211.z;
                break;
            } while(false);
            float2 _239 = (abs(_216) - _213) + float2(_235);
            float _247 = (precise::min(precise::max(_239.x, _239.y), 0.0) + length(precise::max(_239, float2(0.0)))) - _235;
            _254 = precise::min(_174, fast::clamp(0.5 - (_247 / precise::max(fwidth(_247), 9.9999997473787516355514526367188e-05)), 0.0, 1.0));
        }
        else
        {
            _254 = _174;
        }
        float4 _310;
        if (instances._m0[in.in_var_INSTANCE].paint.z != 0u)
        {
            float _292;
            if (instances._m0[in.in_var_INSTANCE].paint.z == 1u)
            {
                float2 _265 = instances._m0[in.in_var_INSTANCE].extra.zw - instances._m0[in.in_var_INSTANCE].extra.xy;
                _292 = dot(in.in_var_LOCAL - instances._m0[in.in_var_INSTANCE].extra.xy, _265) / precise::max(dot(_265, _265), 9.9999999747524270787835121154785e-07);
            }
            else
            {
                float _291;
                if (instances._m0[in.in_var_INSTANCE].paint.z == 2u)
                {
                    _291 = length((in.in_var_LOCAL - instances._m0[in.in_var_INSTANCE].extra.xy) / precise::max(instances._m0[in.in_var_INSTANCE].extra.zw, float2(0.001000000047497451305389404296875)));
                }
                else
                {
                    float2 _282 = in.in_var_LOCAL - instances._m0[in.in_var_INSTANCE].extra.xy;
                    _291 = fract((precise::atan2(_282.x, -_282.y) - instances._m0[in.in_var_INSTANCE].extra.z) * 0.15915493667125701904296875);
                }
                _292 = _291;
            }
            _310 = rampTexture.sample(linearSampler, float2(((fast::clamp(_292, 0.0, 1.0) * 255.0) + 0.5) * 0.00390625, (float(instances._m0[in.in_var_INSTANCE].paint.y) + 0.5) / float(max(frame.rampRows, 1u))), level(0.0));
        }
        else
        {
            _310 = float4(float(instances._m0[in.in_var_INSTANCE].fill & 255u), float((instances._m0[in.in_var_INSTANCE].fill >> 8u) & 255u), float((instances._m0[in.in_var_INSTANCE].fill >> 16u) & 255u), float(instances._m0[in.in_var_INSTANCE].fill >> 24u)) * float4(0.0039215688593685626983642578125);
        }
        _852 = (_310 * _254) + ((float4(float(instances._m0[in.in_var_INSTANCE].stroke & 255u), float((instances._m0[in.in_var_INSTANCE].stroke >> 8u) & 255u), float((instances._m0[in.in_var_INSTANCE].stroke >> 16u) & 255u), float(instances._m0[in.in_var_INSTANCE].stroke >> 24u)) * float4(0.0039215688593685626983642578125)) * fast::clamp(_174 - _254, 0.0, 1.0));
    }
    else
    {
        float4 _851;
        if (instances._m0[in.in_var_INSTANCE].kind == 1u)
        {
            float2 _333 = instances._m0[in.in_var_INSTANCE].rect.zw * 0.5;
            float2 _336 = (in.in_var_LOCAL - instances._m0[in.in_var_INSTANCE].rect.xy) - _333;
            float _355;
            do
            {
                if (_336.x < 0.0)
                {
                    if (_336.y < 0.0)
                    {
                        _355 = instances._m0[in.in_var_INSTANCE].radii.x;
                        break;
                    }
                    _355 = instances._m0[in.in_var_INSTANCE].radii.w;
                    break;
                }
                if (_336.y < 0.0)
                {
                    _355 = instances._m0[in.in_var_INSTANCE].radii.y;
                    break;
                }
                _355 = instances._m0[in.in_var_INSTANCE].radii.z;
                break;
            } while(false);
            float2 _359 = (abs(_336) - _333) + float2(_355);
            float _367 = (precise::min(precise::max(_359.x, _359.y), 0.0) + length(precise::max(_359, float2(0.0)))) - _355;
            float _368 = fwidth(_367);
            float _509;
            do
            {
                float _397;
                float2 _390 = precise::max(_333 + float2(instances._m0[in.in_var_INSTANCE].extra.z), float2(0.0));
                float2 _394 = in.in_var_LOCAL - ((instances._m0[in.in_var_INSTANCE].rect.xy + _333) + instances._m0[in.in_var_INSTANCE].extra.xy);
                float _413;
                do
                {
                    _397 = _394.x;
                    if (_397 < 0.0)
                    {
                        if (_394.y < 0.0)
                        {
                            _413 = instances._m0[in.in_var_INSTANCE].radii.x;
                            break;
                        }
                        _413 = instances._m0[in.in_var_INSTANCE].radii.w;
                        break;
                    }
                    if (_394.y < 0.0)
                    {
                        _413 = instances._m0[in.in_var_INSTANCE].radii.y;
                        break;
                    }
                    _413 = instances._m0[in.in_var_INSTANCE].radii.z;
                    break;
                } while(false);
                float _416 = _390.x;
                float _417 = _390.y;
                float _419 = precise::min(precise::max(_413 + instances._m0[in.in_var_INSTANCE].extra.z, 0.0), precise::min(_416, _417));
                float2 _423 = (abs(_394) - _390) + float2(_419);
                float _431 = (precise::min(precise::max(_423.x, _423.y), 0.0) + length(precise::max(_423, float2(0.0)))) - _419;
                float _432 = fwidth(_431);
                if (instances._m0[in.in_var_INSTANCE].param < 0.5)
                {
                    _509 = fast::clamp(0.5 - (_431 / precise::max(_432, 9.9999997473787516355514526367188e-05)), 0.0, 1.0);
                    break;
                }
                float _440 = _394.y;
                float _441 = _440 - _417;
                float _442 = _440 + _417;
                float _444 = fast::clamp((-3.0) * instances._m0[in.in_var_INSTANCE].param, _441, _442);
                float _447 = fast::clamp(3.0 * instances._m0[in.in_var_INSTANCE].param, _441, _442) - _444;
                float _448 = _447 * 0.25;
                float _450 = _444 + (_447 * 0.125);
                float _455;
                _455 = 0.0;
                float _452 = _450;
                int _457 = 0;
                for (; _457 < 4; )
                {
                    float _465 = precise::min((_417 - _419) - abs(_440 - _452), 0.0);
                    float _472 = (_416 - _419) + sqrt(precise::max(0.0, (_419 * _419) - (_465 * _465)));
                    float2 _478 = (float2(_397) + float2(-_472, _472)) * (0.707106769084930419921875 / instances._m0[in.in_var_INSTANCE].param);
                    float2 _481 = float2(int2(sign(_478)));
                    float2 _482 = abs(_478);
                    float2 _489 = float2(1.0) + ((float2(0.2783930003643035888671875) + ((float2(0.23038899898529052734375) + ((_482 * _482) * 0.07810799777507781982421875)) * _482)) * _482);
                    float2 _490 = _489 * _489;
                    float2 _495 = float2(0.5) + ((_481 - (_481 / (_490 * _490))) * 0.5);
                    float _499 = _452 * _452;
                    _452 += _448;
                    _455 += (((_495.y - _495.x) * (exp((-_499) / ((2.0 * instances._m0[in.in_var_INSTANCE].param) * instances._m0[in.in_var_INSTANCE].param)) / (2.5066282749176025390625 * instances._m0[in.in_var_INSTANCE].param))) * _448);
                    _457++;
                    continue;
                }
                _509 = _455;
                break;
            } while(false);
            _851 = ((float4(float(instances._m0[in.in_var_INSTANCE].fill & 255u), float((instances._m0[in.in_var_INSTANCE].fill >> 8u) & 255u), float((instances._m0[in.in_var_INSTANCE].fill >> 16u) & 255u), float(instances._m0[in.in_var_INSTANCE].fill >> 24u)) * float4(0.0039215688593685626983642578125)) * _509) * (1.0 - fast::clamp(0.5 - (_367 / precise::max(_368, 9.9999997473787516355514526367188e-05)), 0.0, 1.0));
        }
        else
        {
            float4 _850;
            if (instances._m0[in.in_var_INSTANCE].kind == 2u)
            {
                float2 _518 = instances._m0[in.in_var_INSTANCE].rect.zw * 0.5;
                float2 _521 = (in.in_var_LOCAL - instances._m0[in.in_var_INSTANCE].rect.xy) - _518;
                float _540;
                do
                {
                    if (_521.x < 0.0)
                    {
                        if (_521.y < 0.0)
                        {
                            _540 = instances._m0[in.in_var_INSTANCE].radii.x;
                            break;
                        }
                        _540 = instances._m0[in.in_var_INSTANCE].radii.w;
                        break;
                    }
                    if (_521.y < 0.0)
                    {
                        _540 = instances._m0[in.in_var_INSTANCE].radii.y;
                        break;
                    }
                    _540 = instances._m0[in.in_var_INSTANCE].radii.z;
                    break;
                } while(false);
                float2 _544 = (abs(_521) - _518) + float2(_540);
                float _552 = (precise::min(precise::max(_544.x, _544.y), 0.0) + length(precise::max(_544, float2(0.0)))) - _540;
                float _553 = fwidth(_552);
                float _695;
                do
                {
                    float _583;
                    float _573 = instances._m0[in.in_var_INSTANCE].extra.z * (-1.0);
                    float2 _576 = precise::max(_518 + float2(_573), float2(0.0));
                    float2 _580 = in.in_var_LOCAL - ((instances._m0[in.in_var_INSTANCE].rect.xy + _518) + instances._m0[in.in_var_INSTANCE].extra.xy);
                    float _599;
                    do
                    {
                        _583 = _580.x;
                        if (_583 < 0.0)
                        {
                            if (_580.y < 0.0)
                            {
                                _599 = instances._m0[in.in_var_INSTANCE].radii.x;
                                break;
                            }
                            _599 = instances._m0[in.in_var_INSTANCE].radii.w;
                            break;
                        }
                        if (_580.y < 0.0)
                        {
                            _599 = instances._m0[in.in_var_INSTANCE].radii.y;
                            break;
                        }
                        _599 = instances._m0[in.in_var_INSTANCE].radii.z;
                        break;
                    } while(false);
                    float _602 = _576.x;
                    float _603 = _576.y;
                    float _605 = precise::min(precise::max(_599 + _573, 0.0), precise::min(_602, _603));
                    float2 _609 = (abs(_580) - _576) + float2(_605);
                    float _617 = (precise::min(precise::max(_609.x, _609.y), 0.0) + length(precise::max(_609, float2(0.0)))) - _605;
                    float _618 = fwidth(_617);
                    if (instances._m0[in.in_var_INSTANCE].param < 0.5)
                    {
                        _695 = fast::clamp(0.5 - (_617 / precise::max(_618, 9.9999997473787516355514526367188e-05)), 0.0, 1.0);
                        break;
                    }
                    float _626 = _580.y;
                    float _627 = _626 - _603;
                    float _628 = _626 + _603;
                    float _630 = fast::clamp((-3.0) * instances._m0[in.in_var_INSTANCE].param, _627, _628);
                    float _633 = fast::clamp(3.0 * instances._m0[in.in_var_INSTANCE].param, _627, _628) - _630;
                    float _634 = _633 * 0.25;
                    float _636 = _630 + (_633 * 0.125);
                    float _641;
                    _641 = 0.0;
                    float _638 = _636;
                    int _643 = 0;
                    for (; _643 < 4; )
                    {
                        float _651 = precise::min((_603 - _605) - abs(_626 - _638), 0.0);
                        float _658 = (_602 - _605) + sqrt(precise::max(0.0, (_605 * _605) - (_651 * _651)));
                        float2 _664 = (float2(_583) + float2(-_658, _658)) * (0.707106769084930419921875 / instances._m0[in.in_var_INSTANCE].param);
                        float2 _667 = float2(int2(sign(_664)));
                        float2 _668 = abs(_664);
                        float2 _675 = float2(1.0) + ((float2(0.2783930003643035888671875) + ((float2(0.23038899898529052734375) + ((_668 * _668) * 0.07810799777507781982421875)) * _668)) * _668);
                        float2 _676 = _675 * _675;
                        float2 _681 = float2(0.5) + ((_667 - (_667 / (_676 * _676))) * 0.5);
                        float _685 = _638 * _638;
                        _638 += _634;
                        _641 += (((_681.y - _681.x) * (exp((-_685) / ((2.0 * instances._m0[in.in_var_INSTANCE].param) * instances._m0[in.in_var_INSTANCE].param)) / (2.5066282749176025390625 * instances._m0[in.in_var_INSTANCE].param))) * _634);
                        _643++;
                        continue;
                    }
                    _695 = _641;
                    break;
                } while(false);
                _850 = ((float4(float(instances._m0[in.in_var_INSTANCE].fill & 255u), float((instances._m0[in.in_var_INSTANCE].fill >> 8u) & 255u), float((instances._m0[in.in_var_INSTANCE].fill >> 16u) & 255u), float(instances._m0[in.in_var_INSTANCE].fill >> 24u)) * float4(0.0039215688593685626983642578125)) * (1.0 - _695)) * fast::clamp(0.5 - (_552 / precise::max(_553, 9.9999997473787516355514526367188e-05)), 0.0, 1.0);
            }
            else
            {
                float4 _849;
                if (instances._m0[in.in_var_INSTANCE].kind == 3u)
                {
                    _849 = (float4(float(instances._m0[in.in_var_INSTANCE].fill & 255u), float((instances._m0[in.in_var_INSTANCE].fill >> 8u) & 255u), float((instances._m0[in.in_var_INSTANCE].fill >> 16u) & 255u), float(instances._m0[in.in_var_INSTANCE].fill >> 24u)) * float4(0.0039215688593685626983642578125)) * atlasTexture.sample(linearSampler, mix(instances._m0[in.in_var_INSTANCE].extra.xy, instances._m0[in.in_var_INSTANCE].extra.zw, (in.in_var_LOCAL - instances._m0[in.in_var_INSTANCE].rect.xy) / instances._m0[in.in_var_INSTANCE].rect.zw), level(0.0)).x;
                }
                else
                {
                    float4 _848;
                    if (instances._m0[in.in_var_INSTANCE].kind == 4u)
                    {
                        float2 _735 = in.in_var_LOCAL - instances._m0[in.in_var_INSTANCE].rect.xy;
                        float2 _743 = instances._m0[in.in_var_INSTANCE].rect.zw * 0.5;
                        float2 _744 = _735 - _743;
                        float _763;
                        do
                        {
                            if (_744.x < 0.0)
                            {
                                if (_744.y < 0.0)
                                {
                                    _763 = instances._m0[in.in_var_INSTANCE].radii.x;
                                    break;
                                }
                                _763 = instances._m0[in.in_var_INSTANCE].radii.w;
                                break;
                            }
                            if (_744.y < 0.0)
                            {
                                _763 = instances._m0[in.in_var_INSTANCE].radii.y;
                                break;
                            }
                            _763 = instances._m0[in.in_var_INSTANCE].radii.z;
                            break;
                        } while(false);
                        float2 _767 = (abs(_744) - _743) + float2(_763);
                        float _775 = (precise::min(precise::max(_767.x, _767.y), 0.0) + length(precise::max(_767, float2(0.0)))) - _763;
                        _848 = imageTexture.sample(linearSampler, mix(instances._m0[in.in_var_INSTANCE].extra.xy, instances._m0[in.in_var_INSTANCE].extra.zw, _735 / instances._m0[in.in_var_INSTANCE].rect.zw), level(0.0)) * fast::clamp(0.5 - (_775 / precise::max(fwidth(_775), 9.9999997473787516355514526367188e-05)), 0.0, 1.0);
                    }
                    else
                    {
                        float4 _801 = float4(float(instances._m0[in.in_var_INSTANCE].fill & 255u), float((instances._m0[in.in_var_INSTANCE].fill >> 8u) & 255u), float((instances._m0[in.in_var_INSTANCE].fill >> 16u) & 255u), float(instances._m0[in.in_var_INSTANCE].fill >> 24u)) * float4(0.0039215688593685626983642578125);
                        float2 _803 = instances._m0[in.in_var_INSTANCE].rect.zw * 0.5;
                        float2 _806 = (in.in_var_LOCAL - instances._m0[in.in_var_INSTANCE].rect.xy) - _803;
                        float _825;
                        do
                        {
                            if (_806.x < 0.0)
                            {
                                if (_806.y < 0.0)
                                {
                                    _825 = instances._m0[in.in_var_INSTANCE].radii.x;
                                    break;
                                }
                                _825 = instances._m0[in.in_var_INSTANCE].radii.w;
                                break;
                            }
                            if (_806.y < 0.0)
                            {
                                _825 = instances._m0[in.in_var_INSTANCE].radii.y;
                                break;
                            }
                            _825 = instances._m0[in.in_var_INSTANCE].radii.z;
                            break;
                        } while(false);
                        float2 _829 = (abs(_806) - _803) + float2(_825);
                        float _837 = (precise::min(precise::max(_829.x, _829.y), 0.0) + length(precise::max(_829, float2(0.0)))) - _825;
                        _848 = (_801 + (backdropTexture.sample(linearSampler, (gl_FragCoord.xy * frame.inverseViewport), level(0.0)) * (1.0 - _801.w))) * fast::clamp(0.5 - (_837 / precise::max(fwidth(_837), 9.9999997473787516355514526367188e-05)), 0.0, 1.0);
                    }
                    _849 = _848;
                }
                _850 = _849;
            }
            _851 = _850;
        }
        _852 = _851;
    }
    float _928;
    do
    {
        if (instances._m0[in.in_var_INSTANCE].clip == 0u)
        {
            _928 = 1.0;
            break;
        }
        uint _863 = min(instances._m0[in.in_var_INSTANCE].clip, (max(frame.clipCount, 1u) - 1u));
        float2 _888 = clips._m0[_863].rect.zw * 0.5;
        float2 _891 = ((float2((clips._m0[_863]._inverse.x * gl_FragCoord.x) + (clips._m0[_863]._inverse.z * gl_FragCoord.y), (clips._m0[_863]._inverse.y * gl_FragCoord.x) + (clips._m0[_863]._inverse.w * gl_FragCoord.y)) + clips._m0[_863].inverseTranslate) - clips._m0[_863].rect.xy) - _888;
        float _910;
        do
        {
            if (_891.x < 0.0)
            {
                if (_891.y < 0.0)
                {
                    _910 = clips._m0[_863].radii.x;
                    break;
                }
                _910 = clips._m0[_863].radii.w;
                break;
            }
            if (_891.y < 0.0)
            {
                _910 = clips._m0[_863].radii.y;
                break;
            }
            _910 = clips._m0[_863].radii.z;
            break;
        } while(false);
        float2 _914 = (abs(_891) - _888) + float2(_910);
        float _922 = (precise::min(precise::max(_914.x, _914.y), 0.0) + length(precise::max(_914, float2(0.0)))) - _910;
        _928 = fast::clamp(0.5 - (_922 / precise::max(fwidth(_922), 9.9999997473787516355514526367188e-05)), 0.0, 1.0);
        break;
    } while(false);
    out.out_var_SV_Target = (_852 * instances._m0[in.in_var_INSTANCE].opacity) * _928;
    return out;
}


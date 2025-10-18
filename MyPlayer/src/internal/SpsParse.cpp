#include "SpsParse.h"

static int U(int iBitCount, const char* bData, int& iStartBit)
{
    int iRet = 0;
    for (int i = 0; i < iBitCount; i++)
    {
        iRet = iRet << 1;
        if ((0x80 >> (iStartBit % 8)) == (bData[iStartBit / 8] & (0x80 >> (iStartBit % 8))))
        {
            iRet += 1;
        }
        iStartBit++;
    }
    return iRet;
}

static unsigned Ue(const char* bData, const int& dataLen, int& iStartBit)
{
    //����0bit�ĸ���
    int nZeroNum = 0;
    while (iStartBit < dataLen * 8)   //���������strlen(bData)ȥ����ΪbData���п��ܴ�����Ч������\0�����
    {
        if ((0x80 >> (iStartBit % 8)) == (bData[iStartBit / 8] & (0x80 >> (iStartBit % 8)))) //&:��λ�룬%ȡ��
        {
            break;
        }
        nZeroNum++;
        iStartBit++;
    }
    nZeroNum = nZeroNum + 1;
    //������
    unsigned dwRet = 0;
    for (unsigned i = 0; i < nZeroNum; i++)
    {
        dwRet <<= 1;
        if ((0x80 >> (iStartBit % 8)) == (bData[iStartBit / 8] & (0x80 >> (iStartBit % 8))))
        {
            dwRet += 1;
        }
        iStartBit++;
    }
    return dwRet - 1;
}

static int Se(const char* bData, const int& dataLen, int& iStartBit)
{
    //����0bit�ĸ���
    int nZeroNum = 0;
    while (iStartBit < dataLen * 8)
    {
        if ((0x80 >> (iStartBit % 8)) == (bData[iStartBit / 8] & (0x80 >> (iStartBit % 8)))) //&:��λ�룬%ȡ��
        {
            break;
        }
        nZeroNum++;
        iStartBit++;
    }
    //������
    int dwRet = 0;
    for (unsigned i = 0; i < nZeroNum; i++)
    {
        dwRet <<= 1;
        if ((0x80 >> (iStartBit % 8)) == (bData[iStartBit / 8] & (0x80 >> (iStartBit % 8))))
        {
            dwRet += 1;
        }
        iStartBit++;
    }
    if ((0x80 >> (iStartBit % 8)) == (bData[iStartBit / 8] & (0x80 >> (iStartBit % 8))))
    {
        dwRet = 0 - dwRet;
    }
    iStartBit++;
    return dwRet;
}

bool H264_decode_sps(const char* bData, const int dataLen, int& width, int& height)
{
    int StartBit = 0;
    int forbidden_zero_bit = U(1, bData, StartBit);
    int nal_ref_idc = U(2, bData, StartBit);
    int nal_unit_type = U(5, bData, StartBit);
    if (nal_unit_type == 7)
    {
        int profile_idc = U(8, bData, StartBit);
        int constraint_set0_flag = U(1, bData, StartBit);//(buf[1] & 0x80)>>7;
        int constraint_set1_flag = U(1, bData, StartBit);//(buf[1] & 0x40)>>6;
        int constraint_set2_flag = U(1, bData, StartBit);//(buf[1] & 0x20)>>5;
        int constraint_set3_flag = U(1, bData, StartBit);//(buf[1] & 0x10)>>4;
        int reserved_zero_4bits = U(4, bData, StartBit);
        int level_idc = U(8, bData, StartBit);
        unsigned int seq_parameter_set_id = Ue(bData, dataLen, StartBit);
        unsigned int chroma_format_idc = 1; //�ο���������0������  https://blog.csdn.net/lizhijian21/article/details/80982403 ����������ʱĬ��1��gjc���˴��޸�Ϊ1
        if (profile_idc == 100 || profile_idc == 110 ||
            profile_idc == 122 || profile_idc == 144)
        {
            chroma_format_idc = Ue(bData, dataLen, StartBit);
            if (chroma_format_idc == 3)
            {
                int residual_colour_transform_flag = U(1, bData, StartBit);
            }
            unsigned int bit_depth_luma_minus8 = Ue(bData, dataLen, StartBit);
            unsigned int bit_depth_chroma_minus8 = Ue(bData, dataLen, StartBit);
            int qpprime_y_zero_transform_bypass_flag = U(1, bData, StartBit);
            int seq_scaling_matrix_present_flag = U(1, bData, StartBit);

            int* seq_scaling_list_present_flag = new int[8];
            if (1 == seq_scaling_matrix_present_flag)
            {
                for (int i = 0; i < 8; i++)
                {
                    seq_scaling_list_present_flag[i] = U(1, bData, StartBit);
                }
            }
            delete[] seq_scaling_list_present_flag;
        }
        unsigned int log2_max_frame_num_minus4 = Ue(bData, dataLen, StartBit);
        unsigned int pic_order_cnt_type = Ue(bData, dataLen, StartBit);
        if (pic_order_cnt_type == 0)
        {
            unsigned int log2_max_pic_order_cnt_lsb_minus4 = Ue(bData, dataLen, StartBit);
        }
        else if (pic_order_cnt_type == 1)
        {
            int delta_pic_order_always_zero_flag = U(1, bData, StartBit);
            int offset_for_non_ref_pic = Se(bData, dataLen, StartBit);
            int offset_for_top_to_bottom_field = Se(bData, dataLen, StartBit);
            unsigned int num_ref_frames_in_pic_order_cnt_cycle = Ue(bData, dataLen, StartBit);

            int* offset_for_ref_frame = new int[num_ref_frames_in_pic_order_cnt_cycle];
            for (int i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++)
                offset_for_ref_frame[i] = Se(bData, dataLen, StartBit);
            delete[] offset_for_ref_frame;
        }
        unsigned int num_ref_frames = Ue(bData, dataLen, StartBit);
        int gaps_in_frame_num_value_allowed_flag = U(1, bData, StartBit);
        unsigned int pic_width_in_mbs_minus1 = Ue(bData, dataLen, StartBit);
        unsigned int pic_height_in_map_units_minus1 = Ue(bData, dataLen, StartBit);


        int frame_mbs_only_flag = U(1, bData, StartBit);
        if (0 == frame_mbs_only_flag)
        {
            int mb_adaptive_frame_field_flag = U(1, bData, StartBit);
        }
        int direct_8x8_inference_flag = U(1, bData, StartBit);
        int frame_cropping_flag = U(1, bData, StartBit);

        unsigned int frame_crop_left_offset = 0;
        unsigned int frame_crop_right_offset = 0;
        unsigned int frame_crop_top_offset = 0;
        unsigned int frame_crop_bottom_offset = 0;

        if (1 == frame_cropping_flag)
        {
            frame_crop_left_offset = Ue(bData, dataLen, StartBit);
            frame_crop_right_offset = Ue(bData, dataLen, StartBit);
            frame_crop_top_offset = Ue(bData, dataLen, StartBit);
            frame_crop_bottom_offset = Ue(bData, dataLen, StartBit);
        }
        int vui_parameter_present_flag = U(1, bData, StartBit);
        if (1 == vui_parameter_present_flag)
        {
            int aspect_ratio_info_present_flag = U(1, bData, StartBit);
            if (1 == aspect_ratio_info_present_flag)
            {
                int aspect_ratio_idc = U(8, bData, StartBit);
                if (aspect_ratio_idc == 255)
                {
                    int sar_width = U(16, bData, StartBit);
                    int sar_height = U(16, bData, StartBit);
                }
            }
            int overscan_info_present_flag = U(1, bData, StartBit);
            if (1 == overscan_info_present_flag)
            {
                int overscan_appropriate_flagu = U(1, bData, StartBit);
            }
            int video_signal_type_present_flag = U(1, bData, StartBit);
            if (1 == video_signal_type_present_flag)
            {
                int video_format = U(3, bData, StartBit);
                int video_full_range_flag = U(1, bData, StartBit);
                int colour_description_present_flag = U(1, bData, StartBit);
                if (1 == colour_description_present_flag)
                {
                    int colour_primaries = U(8, bData, StartBit);
                    int transfer_characteristics = U(8, bData, StartBit);
                    int matrix_coefficients = U(8, bData, StartBit);
                }
            }
            int chroma_loc_info_present_flag = U(1, bData, StartBit);
            if (1 == chroma_loc_info_present_flag)
            {
                unsigned int chroma_sample_loc_type_top_field = Ue(bData, dataLen, StartBit);
                unsigned int chroma_sample_loc_type_bottom_field = Ue(bData, dataLen, StartBit);
            }
            int timing_info_present_flag = U(1, bData, StartBit);

            if (1 == timing_info_present_flag)
            {
                int num_units_in_tick = U(32, bData, StartBit);
                int time_scale = U(32, bData, StartBit);
                int fixed_frame_rate_flag = U(1, bData, StartBit);
            }

        }

        // ���߼��㹫ʽ
        width = ((int)pic_width_in_mbs_minus1 + 1) * 16;
        height = (2 - (int)frame_mbs_only_flag) * ((int)pic_height_in_map_units_minus1 + 1) * 16;

        if (1 == frame_cropping_flag)
        {
            int crop_unit_x;
            int crop_unit_y;

            if (0 == chroma_format_idc) // monochrome
            {
                crop_unit_x = 1;
                crop_unit_y = 2 - frame_mbs_only_flag;
            }
            else if (1 == chroma_format_idc) // 4:2:0
            {
                crop_unit_x = 2;
                crop_unit_y = 2 * (2 - frame_mbs_only_flag);
            }
            else if (2 == chroma_format_idc) // 4:2:2
            {
                crop_unit_x = 2;
                crop_unit_y = 2 - frame_mbs_only_flag;
            }
            else // 3 == sps.chroma_format_idc   // 4:4:4
            {
                crop_unit_x = 1;
                crop_unit_y = 2 - frame_mbs_only_flag;
            }

            width -= crop_unit_x * ((int)frame_crop_left_offset + (int)frame_crop_right_offset);
            height -= crop_unit_y * ((int)frame_crop_top_offset + (int)frame_crop_bottom_offset);
        }
        return true;
    }
    else
    {
        return false;
    }
}
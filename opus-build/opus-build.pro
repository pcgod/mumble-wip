include(../compiler.pri)

BUILDDIR=$$basename(PWD)
SOURCEDIR=$$replace(BUILDDIR,-build,-src)

!exists(../$$SOURCEDIR/COPYING) {
	message("The $$SOURCEDIR/ directory was not found. You need to do one of the following:")
	message("")
	message("Option 1: Use CELT Git:")
	message("git submodule init")
	message("git submodule update")
	message("")
	message("Option 2: Use system celt libraries (it's your job to ensure you have all of them):")
	message("qmake CONFIG+=no-bundled-celt -recursive")
	message("")
	error("Aborting configuration")
}

TEMPLATE = lib
CONFIG -= qt
CONFIG += debug_and_release
CONFIG -= warn_on
CONFIG += warn_off
VPATH	= ../$$SOURCEDIR
TARGET = opus
DEFINES += HAVE_CONFIG_H OPUS_BUILD inline=__inline

QMAKE_CFLAGS -= -fPIE -pie

win32 {
  DEFINES += WIN32 _WIN32
  INCLUDEPATH += ../$$BUILDDIR/win32

  CONFIG(sse2) {
    TARGET_VERSION_EXT = .$${VERSION}.sse2
  } else {
    QMAKE_CFLAGS_RELEASE -= -arch:SSE
    QMAKE_CFLAGS_DEBUG -= -arch:SSE
  }
}

unix {
	INCLUDEPATH += ../$$BUILDDIR
}

DIST = config.h

INCLUDEPATH *= \
../$$SOURCEDIR/libcelt \
../$$SOURCEDIR/silk \
../$$SOURCEDIR/silk/float

SOURCES *= \
libcelt/bands.c \
libcelt/celt.c \
libcelt/cwrs.c \
libcelt/entcode.c \
libcelt/entdec.c \
libcelt/entenc.c \
libcelt/kiss_fft.c \
libcelt/laplace.c \
libcelt/mathops.c \
libcelt/mdct.c \
libcelt/modes.c \
libcelt/pitch.c \
libcelt/plc.c \
libcelt/quant_bands.c \
libcelt/rate.c \
libcelt/vq.c

SOURCES *= \
silk/float/SKP_Silk_apply_sine_window_FLP.c \
silk/float/SKP_Silk_autocorrelation_FLP.c \
silk/float/SKP_Silk_burg_modified_FLP.c \
silk/float/SKP_Silk_bwexpander_FLP.c \
silk/float/SKP_Silk_corrMatrix_FLP.c \
silk/float/SKP_Silk_encode_frame_FLP.c \
silk/float/SKP_Silk_energy_FLP.c \
silk/float/SKP_Silk_find_LPC_FLP.c \
silk/float/SKP_Silk_find_LTP_FLP.c \
silk/float/SKP_Silk_find_pitch_lags_FLP.c \
silk/float/SKP_Silk_find_pred_coefs_FLP.c \
silk/float/SKP_Silk_inner_product_FLP.c \
silk/float/SKP_Silk_k2a_FLP.c \
silk/float/SKP_Silk_levinsondurbin_FLP.c \
silk/float/SKP_Silk_LPC_analysis_filter_FLP.c \
silk/float/SKP_Silk_LPC_inv_pred_gain_FLP.c \
silk/float/SKP_Silk_LTP_analysis_filter_FLP.c \
silk/float/SKP_Silk_LTP_scale_ctrl_FLP.c \
silk/float/SKP_Silk_noise_shape_analysis_FLP.c \
silk/float/SKP_Silk_pitch_analysis_core_FLP.c \
silk/float/SKP_Silk_prefilter_FLP.c \
silk/float/SKP_Silk_process_gains_FLP.c \
silk/float/SKP_Silk_regularize_correlations_FLP.c \
silk/float/SKP_Silk_residual_energy_FLP.c \
silk/float/SKP_Silk_scale_copy_vector_FLP.c \
silk/float/SKP_Silk_scale_vector_FLP.c \
silk/float/SKP_Silk_schur_FLP.c \
silk/float/SKP_Silk_solve_LS_FLP.c \
silk/float/SKP_Silk_sort_FLP.c \
silk/float/SKP_Silk_warped_autocorrelation_FLP.c \
silk/float/SKP_Silk_wrappers_FLP.c \
silk/SKP_Silk_A2NLSF.c \
silk/SKP_Silk_ana_filt_bank_1.c \
silk/SKP_Silk_apply_sine_window.c \
silk/SKP_Silk_array_maxabs.c \
silk/SKP_Silk_autocorr.c \
silk/SKP_Silk_biquad_alt.c \
silk/SKP_Silk_burg_modified.c \
silk/SKP_Silk_bwexpander.c \
silk/SKP_Silk_bwexpander_32.c \
silk/SKP_Silk_check_control_input.c \
silk/SKP_Silk_CNG.c \
silk/SKP_Silk_code_signs.c \
silk/SKP_Silk_control_audio_bandwidth.c \
silk/SKP_Silk_control_codec.c \
silk/SKP_Silk_control_SNR.c \
silk/SKP_Silk_create_init_destroy.c \
silk/SKP_Silk_dec_API.c \
silk/SKP_Silk_decode_core.c \
silk/SKP_Silk_decode_frame.c \
silk/SKP_Silk_decode_indices.c \
silk/SKP_Silk_decode_parameters.c \
silk/SKP_Silk_decode_pitch.c \
silk/SKP_Silk_decode_pulses.c \
silk/SKP_Silk_decoder_set_fs.c \
silk/SKP_Silk_enc_API.c \
silk/SKP_Silk_encode_indices.c \
silk/SKP_Silk_encode_pulses.c \
silk/SKP_Silk_gain_quant.c \
silk/SKP_Silk_HP_variable_cutoff.c \
silk/SKP_Silk_init_encoder.c \
silk/SKP_Silk_inner_prod_aligned.c \
silk/SKP_Silk_interpolate.c \
silk/SKP_Silk_k2a.c \
silk/SKP_Silk_k2a_Q16.c \
silk/SKP_Silk_lin2log.c \
silk/SKP_Silk_log2lin.c \
silk/SKP_Silk_LP_variable_cutoff.c \
silk/SKP_Silk_LPC_analysis_filter.c \
silk/SKP_Silk_LPC_inv_pred_gain.c \
silk/SKP_Silk_LPC_stabilize.c \
silk/SKP_Silk_LPC_synthesis_filter.c \
silk/SKP_Silk_LPC_synthesis_order16.c \
silk/SKP_Silk_LSF_cos_table.c \
silk/SKP_Silk_NLSF2A.c \
silk/SKP_Silk_NLSF2A_stable.c \
silk/SKP_Silk_NLSF_decode.c \
silk/SKP_Silk_NLSF_del_dec_quant.c \
silk/SKP_Silk_NLSF_encode.c \
silk/SKP_Silk_NLSF_stabilize.c \
silk/SKP_Silk_NLSF_unpack.c \
silk/SKP_Silk_NLSF_VQ.c \
silk/SKP_Silk_NLSF_VQ_weights_laroia.c \
silk/SKP_Silk_NSQ.c \
silk/SKP_Silk_NSQ_del_dec.c \
silk/SKP_Silk_pitch_analysis_core.c \
silk/SKP_Silk_pitch_est_tables.c \
silk/SKP_Silk_PLC.c \
silk/SKP_Silk_process_NLSFs.c \
silk/SKP_Silk_quant_LTP_gains.c \
silk/SKP_Silk_resampler.c \
silk/SKP_Silk_resampler_down2.c \
silk/SKP_Silk_resampler_down2_3.c \
silk/SKP_Silk_resampler_down3.c \
silk/SKP_Silk_resampler_private_AR2.c \
silk/SKP_Silk_resampler_private_ARMA4.c \
silk/SKP_Silk_resampler_private_copy.c \
silk/SKP_Silk_resampler_private_down4.c \
silk/SKP_Silk_resampler_private_down_FIR.c \
silk/SKP_Silk_resampler_private_IIR_FIR.c \
silk/SKP_Silk_resampler_private_up2_HQ.c \
silk/SKP_Silk_resampler_private_up4.c \
silk/SKP_Silk_resampler_rom.c \
silk/SKP_Silk_resampler_up2.c \
silk/SKP_Silk_scale_copy_vector16.c \
silk/SKP_Silk_scale_vector.c \
silk/SKP_Silk_schur.c \
silk/SKP_Silk_schur64.c \
silk/SKP_Silk_shell_coder.c \
silk/SKP_Silk_sigm_Q15.c \
silk/SKP_Silk_sort.c \
silk/SKP_Silk_stereo_decode_pred.c \
silk/SKP_Silk_stereo_encode_pred.c \
silk/SKP_Silk_stereo_find_predictor.c \
silk/SKP_Silk_stereo_LR_to_MS.c \
silk/SKP_Silk_stereo_MS_to_LR.c \
silk/SKP_Silk_sum_sqr_shift.c \
silk/SKP_Silk_tables_gain.c \
silk/SKP_Silk_tables_LTP.c \
silk/SKP_Silk_tables_NLSF_CB_NB_MB.c \
silk/SKP_Silk_tables_NLSF_CB_WB.c \
silk/SKP_Silk_tables_other.c \
silk/SKP_Silk_tables_pitch_lag.c \
silk/SKP_Silk_tables_pulses_per_block.c \
silk/SKP_Silk_VAD.c \
silk/SKP_Silk_VQ_WMat_EC.c

SOURCES *= \
src/opus_decoder.c \
src/opus_encoder.c

CONFIG(debug, debug|release) {
  CONFIG += console
  DESTDIR	= ../debug
}

CONFIG(release, debug|release) {
  DESTDIR	= ../release
}

include(../symbols.pri)

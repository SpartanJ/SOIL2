
static int stbi_test_main(stbi *s)
{
   if (stbi_jpeg_test(s)) return STBI_jpeg;
   if (stbi_png_test(s))  return STBI_png;
   if (stbi_bmp_test(s))  return STBI_bmp;
   if (stbi_gif_test(s))  return STBI_gif;
   if (stbi_psd_test(s))  return STBI_psd;
   if (stbi_pic_test(s))  return STBI_pic;
   #ifndef STBI_NO_DDS
   if (stbi_dds_test(s))  return STBI_dds;
   #endif
   #ifndef STBI_NO_PVR
   if (stbi_pvr_test(s))  return STBI_pvr;
   #endif
   #ifndef STBI_NO_PKM
   if (stbi_pkm_test(s))  return STBI_pkm;
   #endif
   #ifndef STBI_NO_HDR
   if (stbi_hdr_test(s))  return STBI_hdr;
   #endif
   if (stbi_tga_test(s))  return STBI_tga;
   return STBI_unknown;
}

#ifndef STBI_NO_STDIO
int stbi_test_from_file(FILE *f)
{
   stbi s;
   start_file(&s,f);
   return stbi_test_main(&s);
}

int stbi_test(char const *filename)
{
   FILE *f = fopen(filename, "rb");
   int result;
   if (!f) return STBI_unknown;
   result = stbi_test_from_file(f);
   fclose(f);
   return result;
}
#endif //!STBI_NO_STDIO

int stbi_test_from_memory(stbi_uc const *buffer, int len)
{
   stbi s;
   start_mem(&s,buffer,len);
   return stbi_test_main(&s);
}

int stbi_test_from_callbacks(stbi_io_callbacks const *clbk, void *user)
{
   stbi s;
   start_callbacks(&s, (stbi_io_callbacks *) clbk, user);
   return stbi_test_main(&s);
}

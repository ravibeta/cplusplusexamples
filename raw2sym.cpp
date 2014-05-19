/Users/rrajamani/Downloads/dia2dump.txt:Function: [00447B60][0001:00446B60] ServerConfig::getSSLConfig(public: struct ssl_config __cdecl ServerConfig::getSSLConfig(void) __ptr64)

Here the RVA is 00447B60  [ eip - Process Load Address ]
         the Segment is 0001
         the offset is 00446B60


        DWORD64  dwAddress = _wcstoui64(argv[i], NULL, 16);
        DWORD64 dwRVA  = dwAddress - dwLoadAddress;
        long displacement = 0;
        IDiaSymbol* pFunc = 0;
        error = (DWORD)g_pDiaSession->findSymbolByRVAEx(dwRVA, SymTagFunction, &pFunc, &displacement );

        if (!error && pFunc)
        {
            BSTR bstrName;

            if (pFunc->get_name(&bstrName) != S_OK) {
                wprintf(L"(???)\n\n");
            }

            else {
                wprintf(L"%s \n\n", bstrName);
                if (displacement)
                    wprintf(L"+ 0x%x \n\n", displacement);
                else
                    wprintf(L" \n\n");
                SysFreeString(bstrName);
            }
        }

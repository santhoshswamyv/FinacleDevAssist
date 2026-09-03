#include "FIExecutionHandler.h"
#include "../FDAClient.h"
#include "../../Utils/Logger.h"
#include "../../nlohmann/json.hpp"
#include "../../Features/ScintillaHelper.h"
#include "../../Notepad_plus_msgs.h"
#include "../../PluginDefinition.h"
#include "../../menuCmdID.h"

using json = nlohmann::json;

FIExecutionHandler::FIExecutionHandler(FDAClient* client)
    : RequestHandler(client)
{
}

void FIExecutionHandler::executeRequest(const FIRequestData& data)
{
    Logger::info("[FI] Preparing request");

    json request;
    json headers;

    headers["Content-Type"] = data.contentType;
    headers["Accept"] = data.accept;

    request["type"] = "EXECUTE_FI_REQUEST";
    request["environment"] = data.environment;
    request["method"] = data.method;
    request["headers"] = headers;
    request["body"] = data.body;

    std::string response;

    if (!client->sendRequest(request.dump(), response))
    {
        Logger::error("[FI] Request failed");
        return;
    }

    Logger::info("[FI] Response received");

    parse(response);

}

void FIExecutionHandler::parse(const std::string& response)
{
    try
    {
        json result = json::parse(response);

        Logger::info("[FI] Response JSON : " + result.dump());

        if (!result.contains("STATUS"))
        {
            Logger::error("[FI] STATUS missing");
            return;
        }

        if (result["STATUS"] != "SUCCESS")
        {
            MessageBox(
                nppData._nppHandle,
                TEXT("Unexpected Error Occurred."),
                TEXT("Finacle Dev Assist"),
                MB_OK | MB_ICONINFORMATION
            );

            if (result.contains("EXCEPTION"))
            {
                if (result["EXCEPTION"].is_string())
                {
                    std::string exceptionMessage = result["EXCEPTION"].get<std::string>();
                    Logger::error("[FI] " + exceptionMessage);
                }
                else
                {
                    Logger::error("[FI] EXCEPTION has unexpected type : " + std::string(result["EXCEPTION"].type_name()));
                }
            }
            else
            {
                Logger::error("[FI] EXCEPTION missing");
            }

            return;
        }

        Logger::info("[FI] SUCCESS received");

        if (!result.contains("response"))
        {
            Logger::error("[FI] response missing");
            return;
        }

        std::string fiResponse = result["response"].get<std::string>();

        Logger::info("[FI] Opening new file");

        ::SendMessage(
            nppData._nppHandle,
            NPPM_MENUCOMMAND,
            0,
            IDM_FILE_NEW
        );

        HWND hNewEditor = ScintillaHelper::getCurrentEditor();

        if (hNewEditor == nullptr)
        {
            Logger::error("[FI] Failed to get new editor handle.");
            return;
        }

        Logger::info("[FI] Setting FI response into new file");

        ::SendMessage(
            hNewEditor,
            SCI_SETTEXT,
            0,
            reinterpret_cast<LPARAM>(fiResponse.c_str())
        );

        ::SendMessage(
            hNewEditor,
            SCI_GOTOPOS,
            0,
            0
        );

        Logger::info("[FI] FI Response showed in new file.");
    }
    catch (const json::parse_error& e)
    {
        Logger::error("[FI] JSON parse failed : " + std::string(e.what()));
    }
    catch (const json::type_error& e)
    {
        Logger::error("[FI] JSON type error : " + std::string(e.what()));
    }
    catch (const std::exception& e)
    {
        Logger::error("[FI] Unexpected error : " + std::string(e.what()));
    }
}
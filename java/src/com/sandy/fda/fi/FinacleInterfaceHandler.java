package com.sandy.fda.fi;

import com.google.gson.Gson;
import com.google.gson.JsonObject;
import com.sandy.fda.beautifier.Beautifier;
import com.sandy.fda.models.fi.FIException;
import com.sandy.fda.models.fi.Request;
import com.sandy.fda.models.fi.Response;
import com.sandy.fda.utils.FDAConstants;
import com.sandy.fda.utils.FDALogger;

public class FinacleInterfaceHandler {

    private FinacleInterfaceClient finacleInterfaceClient;
    private Beautifier beautifier;

    public FinacleInterfaceHandler(Beautifier beautifier) throws Exception {
        this.beautifier = beautifier;
        this.finacleInterfaceClient = new FinacleInterfaceClient();
    }

    public JsonObject execute(JsonObject req) throws Exception {
        String environment = req.remove("environment").getAsString();
        String endpoint = FDAConstants.getProperties().get(environment);
        req.addProperty("endpoint", endpoint);

        Request request = new Gson().fromJson(req, Request.class);
        try {
            Response response = finacleInterfaceClient.send(request);
            return convertToJsonResponse(response);
        } catch (FIException e) {
            FDALogger.error(e);
            return convertToJsonResponse(e);
        }
    }

    private JsonObject beautifyXML(String xmlContent) throws Exception {
        JsonObject beautifyData = new JsonObject();
        beautifyData.addProperty("contentType", "XML");
        beautifyData.addProperty("content", xmlContent);
        return beautifier.beautifyCode(beautifyData);
    }

    private JsonObject convertToJsonResponse(Response response) throws Exception {
        JsonObject jsonResponse = beautifyXML(response.getBody());
        
        jsonResponse.addProperty("STATUS", "SUCCESS");
        
        StringBuilder beautifiedXml = new StringBuilder();
        beautifiedXml.append("HTTP Status Code :- ")
                    .append(response.getStatusCode())
                    .append(System.lineSeparator())
                    .append(System.lineSeparator())
                    .append(jsonResponse.remove("beautifiedCode").getAsString());
        
        jsonResponse.addProperty("response", beautifiedXml.toString());
        return jsonResponse;
    }

    private JsonObject convertToJsonResponse(FIException exception) {
    JsonObject jsonResponse = new JsonObject();

    jsonResponse.addProperty("STATUS", "EXCEPTION");
    jsonResponse.addProperty("EXCEPTION", exception.getMessage());

    return jsonResponse;
}
}

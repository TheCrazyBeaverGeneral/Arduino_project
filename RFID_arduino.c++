#include <WebServer.h>
#include <SPI.h>
#include <MFRC522.h>
#include <FS.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <vector>
#include <algorithm>
#include <WiFi.h>

// WiFi credentials
const char* ssid = "iPhone van Donny";
const char* password = "peepeepee";

// RFID configuration
#define RST_PIN 4
#define SS_PIN 5
MFRC522 mfrc522(SS_PIN, RST_PIN);

// Webserver on port 80
WebServer server(80);

struct Tag {
    String id;
    String name;
    String location;
};

struct Item {
    String name;
};

std::vector<Tag> tags;
std::vector<Item> items;

String getHTML() {
    String html = "<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'>"
                  "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
                  "<title>RFID Management</title>"
                  "<link href='https://cdn.jsdelivr.net/npm/bootstrap@5.0.0-beta2/dist/css/bootstrap.min.css' rel='stylesheet'>"
                  "<script src='https://code.jquery.com/jquery-3.5.1.min.js'></script>"
                  "<style> body { background-color: #f8f9fa; } "
                  ".navbar { margin-bottom: 20px; } "
                  ".table th, .table td { text-align: center; } "
                  ".btn-custom { background-color: #007bff; border-color: #007bff; } "
                  ".btn-custom:hover { background-color: #0056b3; border-color: #0056b3; }"
                  "</style></head><body>"
                  "<nav class='navbar navbar-expand-lg navbar-light bg-light'>"
                  "<a class='navbar-brand' href='/'>RFID Management</a>"
                  "<a class='navbar-brand' href='/items'>Items Management</a>"
                  "</nav>"
                  "<div class='container mt-5'><h1 class='text-center'>Spot Opslagsysteem</h1>"
                  "<div class='card'><div class='card-body'>"
                  "<table class='table table-bordered'><thead><tr><th>Tag ID</th><th>Assigned Name</th><th>Location</th><th>Action</th></tr></thead>"
                  "<tbody id='tagTable'>";

    // Add rows for each tag
    for (const auto& tag : tags) {
        html += "<tr id='tag_" + tag.id + "'>";
        html += "<td>" + tag.id + "</td>";
        html += "<td><select class='form-select' id='assignedName_" + tag.id + "'>";

        // Add dropdown options for assigned names from items
        for (const auto& item : items) {
            html += "<option value='" + item.name + "'" + (tag.name == item.name ? " selected" : "") + ">" + item.name + "</option>";
        }

        html += "</select></td>"; // Close select tag

        // Dropdown for location selection
        html += "<td><select class='form-select' id='location_" + tag.id + "'>"
                "<option value='Opslagruimte één'" + (tag.location == "Opslagruimte één" ? " selected" : "") + ">Opslagruimte één</option>"
                "<option value='Opslagruimte twee'" + (tag.location == "Opslagruimte twee" ? " selected" : "") + ">Opslagruimte twee</option>"
                "</select></td>";

        // Action buttons for save and delete
        html += "<td><button class='btn btn-custom' onclick='saveTag(\"" + tag.id + "\")'>Save</button>"
                "<button class='btn btn-danger' onclick='deleteTag(\"" + tag.id + "\")'>Delete</button></td>";
        html += "</tr>";
    }

    html += "</tbody></table></div></div></div>"
            "<script>"
            "const items = " + getItemsJson() + ";"
            "function saveTag(tagId) {"
            "  var selectedName = document.getElementById('assignedName_' + tagId).value;"
            "  var selectedLocation = document.getElementById('location_' + tagId).value;" 
            "  fetch('/saveTag', {"
            "    method: 'POST',"
            "    headers: { 'Content-Type': 'application/json' },"
            "    body: JSON.stringify({ id: tagId, name: selectedName, location: selectedLocation })"
            "  }).then(response => response.text()).then(alert);"
            "} "          
            "function deleteTag(tagId) {"
            "  fetch('/deleteTag', {"
            "    method: 'POST',"
            "    headers: { 'Content-Type': 'application/json' },"
            "    body: JSON.stringify({ id: tagId })"
            "  }).then(response => response.text()).then(alert);"
            "} "
            "function refreshTagTable() {"
            "  fetch('/getTags', {"
            "    method: 'GET'"
            "  }).then(response => response.json()).then(data => {"
            "    let tagTable = document.getElementById('tagTable');"
            "    tagTable.innerHTML = '';"
            "    data.forEach(tag => {"
            "      let row = '<tr id=\"tag_' + tag.id + '\">'"
            "                   + '<td>' + tag.id + '</td>'"
            "                   + '<td><select class=\"form-select\" id=\"assignedName_' + tag.id + '\">'"
            "                       + items.map(item => '<option value=\"' + item.name + '\"' + (tag.name === item.name ? ' selected' : '') + '>' + item.name + '</option>').join('')"
            "                   + '</select></td>'"
            "                   + '<td><select class=\"form-select\" id=\"location_' + tag.id + '\">'"
            "                     + '<option value=\"Opslagruimte één\"' + (tag.location === 'Opslagruimte één' ? ' selected' : '') + '>Opslagruimte één</option>'"
            "                     + '<option value=\"Opslagruimte twee\"' + (tag.location === 'Opslagruimte twee' ? ' selected' : '') + '>Opslagruimte twee</option>'"
            "                   + '</select></td>'"
            "                   + '<td><button class=\"btn btn-custom\" onclick=\"saveTag(`' + tag.id + '`)\">Save</button>'"
            "                       + '<button class=\"btn btn-danger\" onclick=\"deleteTag(`' + tag.id + '`)\">Delete</button></td>'"
            "                 + '</tr>';"
            "      tagTable.innerHTML += row;"
            "    });"
            "  });"
            "} "
            // Refresh the table every 10 seconds
            "setInterval(refreshTagTable, 10000);"
            "refreshTagTable();" // Initial call to populate the table
            "</script></body></html>";

    return html;
}

String getItemsJson() {
    DynamicJsonDocument doc(1024);
    JsonArray array = doc.to<JsonArray>();

    for (const auto& item : items) {
        JsonObject obj = array.createNestedObject();
        obj["name"] = item.name;
    }

    String jsonResponse;
    serializeJson(doc, jsonResponse);
    return jsonResponse;
}

String getItemsHTML() {
    String html = "<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'>"
                  "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
                  "<title>Items Management</title>"
                  "<link href='https://cdn.jsdelivr.net/npm/bootstrap@5.0.0-beta2/dist/css/bootstrap.min.css' rel='stylesheet'>"
                  "<script src='https://code.jquery.com/jquery-3.5.1.min.js'></script>"
                  "<style> body { background-color: #f8f9fa; } </style></head><body>"
                  "<nav class='navbar navbar-expand-lg navbar-light bg-light'>"
                  "<a class='navbar-brand' href='/items'>Items Management</a>"
                  "<a class='navbar-brand' href='/'>RFID Management</a>"
                  "</nav>"
                  "<div class='container mt-5'><h1 class='text-center'>Manage Items</h1>"
                  "<div class='card'><div class='card-body'>"
                  "<input type='text' id='newItemName' class='form-control' placeholder='Enter item name'/>"
                  "<button class='btn btn-primary mt-2' onclick='addItem()'>Add Item</button>"
                  "<table class='table table-bordered mt-3'><thead><tr><th>Item Name</th><th>Actions</th></tr></thead><tbody id='itemTable'>";

    // Show all items
    for (const auto& item : items) {
        html += "<tr id='item_" + item.name + "'>"
                "<td><input type='text' id='editItemName_" + item.name + "' value='" + item.name + "' style='width:70%;'/></td>"
                "<td>"
                "<button class='btn btn-primary' onclick='editItem(`" + item.name + "`)'>Edit</button>"
                "<button class='btn btn-danger' onclick='deleteItem(`" + item.name + "`)'>Delete</button>"
                "</td></tr>";
    }

    html += "</tbody></table></div></div></div>"
            "<script>"
            "function addItem() {"
            "  var itemName = document.getElementById('newItemName').value;"
            "  fetch('/addItem', {"
            "    method: 'POST',"
            "    headers: { 'Content-Type': 'application/json' },"
            "    body: JSON.stringify({ name: itemName })"
            "  }).then(response => response.text()).then(data => {"
            "      alert(data);"
            "      document.getElementById('newItemName').value = '';"
            "      location.reload();" // Refresh the page to show updated items
            "  });"
            "} "
            "function deleteItem(itemName) {"
            "  fetch('/deleteItem', {"
            "    method: 'POST',"
            "    headers: { 'Content-Type': 'application/json' },"
            "    body: JSON.stringify({ name: itemName })"
            "  }).then(response => response.text()).then(data => {"
            "      alert(data);"
            "      setTimeout(() => window.location.reload(), 500);"
            "  });"
            "} "
            "function editItem(itemName) {"
            "  var newItemName = document.getElementById('editItemName_' + itemName).value;"
            "  fetch('/editItem', {"
            "    method: 'POST',"
            "    headers: { 'Content-Type': 'application/json' },"
            "    body: JSON.stringify({ oldName: itemName, newName: newItemName })"
            "  }).then(response => response.text()).then(data => {"
            "      alert(data);"
            "      location.reload();"
            "  });"
            "} "
            "</script></body></html>";
    return html;
}

// Save tags to SPIFFS as JSON
void saveTagsToSPIFFS() {
    DynamicJsonDocument doc(1024);
    JsonArray array = doc.to<JsonArray>();

    for (const auto& tag : tags) {
        JsonObject obj = array.createNestedObject();
        obj["id"] = tag.id;
        obj["name"] = tag.name;
        obj["location"] = tag.location;
    }

    File file = SPIFFS.open("/tags.json", FILE_WRITE);
    if (file) {
        serializeJson(doc, file);
        file.close();
    } else {
        Serial.println("Failed to open file for writing");
    }
}

// Load tags from SPIFFS
void loadTagsFromSPIFFS() {
    File file = SPIFFS.open("/tags.json", FILE_READ);
    if (!file) {
        Serial.println("Failed to open file for reading");
        return;
    }

    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        Serial.println("Failed to read file, using default tags");
        return;
    }

    tags.clear();

    JsonArray array = doc.as<JsonArray>();
    for (JsonObject obj : array) {
        Tag tag;
        tag.id = obj["id"].as<String>();
        tag.name = obj["name"].as<String>();
        tag.location = obj["location"].as<String>();
        tags.push_back(tag);
    }
}

// Save items to SPIFFS as JSON
void saveItemsToSPIFFS() {
    DynamicJsonDocument doc(1024);
    JsonArray array = doc.to<JsonArray>();

    for (const auto& item : items) {
        JsonObject obj = array.createNestedObject();
        obj["name"] = item.name;
    }

    File file = SPIFFS.open("/items.json", FILE_WRITE);
    if (file) {
        serializeJson(doc, file);
        file.close();
    } else {
        Serial.println("Failed to open file for writing");
    }
}

// Load items from SPIFFS
void loadItemsFromSPIFFS() {
    File file = SPIFFS.open("/items.json", FILE_READ);
    if (!file) {
        Serial.println("Failed to open file for reading");
        return;
    }

    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        Serial.println("Failed to read file, using default items");
        return;
    }

    items.clear();

    JsonArray array = doc.as<JsonArray>();
    for (JsonObject obj : array) {
        Item item;
        item.name = obj["name"].as<String>();
        items.push_back(item);
    }
}

// Handle the root endpoint
void handleRoot() {
    server.send(200, "text/html", getHTML());
}

// Handle items page
void handleItems() {
    server.send(200, "text/html", getItemsHTML());
}

// Handle adding an item
void handleAddItem() {
    if (server.hasArg("plain")) {
        DynamicJsonDocument doc(256);
        deserializeJson(doc, server.arg("plain"));

        String itemName = doc["name"];
        items.push_back({itemName}); // Add new item to vector

        // Save items to SPIFFS
        saveItemsToSPIFFS();

        server.send(200, "text/plain", "Item added!");
    } else {
        server.send(400, "text/plain", "Invalid request");
    }
}

// Handle saving tag updates
void handleSaveTag() {
    if (server.hasArg("plain")) {
        DynamicJsonDocument doc(512);
        deserializeJson(doc, server.arg("plain"));

        String id = doc["id"];
        String name = doc["name"];
        String location = doc["location"];

        // Update the tag in the list
        for (auto& tag : tags) {
            if (tag.id == id) {
                tag.name = name;
                tag.location = location;
                break;
            }
        }

        // Save updated tags to file
        saveTagsToSPIFFS();

        // Send a response back to the client
        server.send(200, "text/plain", "Tag saved!");
    } else {
        server.send(400, "text/plain", "Invalid request");
    }
}

// Handle deleting a tag
void handleDeleteTag() {
    if (server.hasArg("plain")) {
        DynamicJsonDocument doc(256);
        deserializeJson(doc, server.arg("plain"));

        String id = doc["id"];

        // Remove the tag from the list if it matches the ID
        tags.erase(std::remove_if(tags.begin(), tags.end(), [&](const Tag& tag) {
            return tag.id == id;
        }), tags.end());

        // Save updated tags to SPIFFS
        saveTagsToSPIFFS();  

        // Send a response back to the client
        server.send(200, "text/plain", "Tag deleted!");
    } else {
        server.send(400, "text/plain", "Invalid request");
    }
}

// Handle deleting an item
void handleDeleteItem() {
    if (server.hasArg("plain")) {
        DynamicJsonDocument doc(256);
        deserializeJson(doc, server.arg("plain"));

        String itemName = doc["name"];

        // Remove the item from the list if it matches the name
        items.erase(std::remove_if(items.begin(), items.end(), [&](const Item& item) {
            return item.name == itemName;
        }), items.end());

        // Save updated items to SPIFFS
        saveItemsToSPIFFS();  

        server.send(200, "text/plain", "Item deleted successfully.");
    } else {
        server.send(400, "text/plain", "Invalid request.");
    }
}

// Handle editing an item
void handleEditItem() {
    if (server.hasArg("plain")) {
        DynamicJsonDocument doc(256);
        deserializeJson(doc, server.arg("plain"));

        String oldName = doc["oldName"];
        String newName = doc["newName"];

        // Update the item in the list
        for (auto& item : items) {
            if (item.name == oldName) {
                item.name = newName;
                break;
            }
        }

        // Save updated items to SPIFFS
        saveItemsToSPIFFS();  

        server.send(200, "text/plain", "Item updated successfully.");
    } else {
        server.send(400, "text/plain", "Invalid request.");
    }
}

// Handle getting the list of tags
void handleGetTags() {
    DynamicJsonDocument doc(1024);
    JsonArray array = doc.to<JsonArray>();

    for (const auto& tag : tags) {
        JsonObject obj = array.createNestedObject();
        obj["id"] = tag.id;
        obj["name"] = tag.name;
        obj["location"] = tag.location; 
    }

    String jsonResponse;
    serializeJson(doc, jsonResponse);
    server.send(200, "application/json", jsonResponse);
}

// Handle scanning new tags
void handleScanTag() {
    String tagID = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
        tagID += String(mfrc522.uid.uidByte[i], HEX);
    }
    Serial.println("Scanned Tag: " + tagID);

    bool tagExists = false;
    for (const auto& tag : tags) {
        if (tag.id == tagID) {
            tagExists = true;
            break;
}
    }

    if (!tagExists) {
        tags.push_back({tagID, "Unassigned", "Opslagruimte één"});
        saveTagsToSPIFFS();
    }

    DynamicJsonDocument doc(1024);
    JsonArray array = doc.to<JsonArray>();

    for (const auto& tag : tags) {
        JsonObject obj = array.createNestedObject();
        obj["id"] = tag.id;
        obj["name"] = tag.name;
        obj["location"] = tag.location; 
    }

    String jsonResponse;
    serializeJson(doc, jsonResponse);
    server.send(200, "application/json", jsonResponse);

    mfrc522.PICC_HaltA();
}

void setup() {
    Serial.begin(115200);

    // Initialize SPIFFS
    Serial.println("Starting SPIFFS...");
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS Mount Failed");
        return;
    }
    Serial.println("SPIFFS initialized!");

    // Load existing tags and items from SPIFFS
    loadTagsFromSPIFFS();
    loadItemsFromSPIFFS();

    // RFID setup
    SPI.begin();
    mfrc522.PCD_Init();
    Serial.println("RFID scanner ready.");

    // WiFi setup
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected to WiFi!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    // Web server setup
    server.on("/", handleRoot);
    server.on("/items", handleItems);
    server.on("/addItem", HTTP_POST, handleAddItem);
    server.on("/saveTag", HTTP_POST, handleSaveTag);
    server.on("/deleteTag", HTTP_POST, handleDeleteTag);
    server.on("/scanTag", HTTP_GET, handleScanTag);
    server.on("/getTags", HTTP_GET, handleGetTags);
    server.on("/deleteItem", HTTP_POST, handleDeleteItem);
    server.on("/editItem", HTTP_POST, handleEditItem); // New endpoint for editing items
    server.begin();
    Serial.println("Web server started.");
}

void loop() {
    server.handleClient();

    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
        handleScanTag();
    }
}
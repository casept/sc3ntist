#pragma once

#include <vector>
#include <map>
#include <QObject>
#include "parser/SCXFile.h"
#include "parser/SupportedGame.h"
#include <QtSql>
#include "enums.h"
#include "projectcontextprovider.h"

class Project : public QObject {
  Q_OBJECT

 public:
  // create new project
  explicit Project(const QString& dbPath, const QString& loadPath,
                   const SupportedGame* game, QObject* parent);
  // open old project
  explicit Project(const QString& dbPath, QObject* parent);
  ~Project();

  std::map<int, std::unique_ptr<SCXFile>>& files() { return _files; }
  int currentFileId() const { return _currentFileId; }
  const SCXFile* currentFile() const;

  IContextProvider* contextProvider() { return &_contextProvider; }

  void switchFile(int id);
  void goToAddress(int fileId, SCXOffset address);
  void focusMemory(VariableRefType type, int var);

  QString getComment(int fileId, SCXOffset address);
  void setComment(int fileId, SCXOffset address, const QString& comment);

  QString getLabelName(int fileId, int labelId);
  void setLabelName(int fileId, int labelId, const QString& name);

  QString getVarName(VariableRefType type, int var);
  void setVarName(const QString& name, VariableRefType type, int var);

  QString getVarComment(VariableRefType type, int var);
  void setVarComment(const QString& comment, VariableRefType type, int var);

  bool getBreakpoint(int fileId, SCXOffset address);
  void setBreakpoint(int fileId, SCXOffset address, bool enabled);

  QString getString(int fileId, int stringId);
  int getStringCount(int fileId);

  std::vector<QString> getVariableNames(VariableRefType type, int var);
  std::vector<std::pair<int, SCXOffset>> getVariableRefs(VariableRefType type,
                                                         int var);
  std::vector<std::pair<int, SCXOffset>> getLabelRefs(int fileId, int labelId);

  int getVariableId(VariableRefType type, const QString& name);
  std::pair<VariableRefType, int> parseVarRefString(const QString& str);

  void importWorklist(const QString& path, const char* encoding);

 signals:
  void fileSwitched(int previousId);
  void focusAddressSwitched(SCXOffset newAddress);
  void focusMemorySwitched(VariableRefType type, int var);
  void commentChanged(int fileId, SCXOffset address, const QString& comment);
  void labelNameChanged(int fileId, int labelId, const QString& name);
  void varNameChanged(VariableRefType type, int var, const QString& name);
  void varCommentChanged(VariableRefType type, int var, const QString& comment);
  void breakpointChanged(int fileId, SCXOffset address, bool enabled);
  void allVarsChanged();

 private:
  const SupportedGame* _game;

  QSqlDatabase _db;
  std::map<int, std::unique_ptr<SCXFile>> _files;
  int _currentFileId = -1;
  bool _inInitialLoad = true;

  bool _batchUpdatingVars = false;

  ProjectContextProvider _contextProvider;

  struct TmpFileData {
    int id;
    QString name;
    uint8_t* data;
    int size;
  };
  bool importMpk(const QString& mpkPath, std::vector<TmpFileData>& dest);
  bool importMlp(const QString& mlpPath, std::vector<TmpFileData>& dest);

  void createDatabase(const QString& path);
  void openDatabase(const QString& path);
  void prepareStmts();
  void analyzeFile(const SCXFile* file);
  void loadFilesFromDb();
  void insertFile(const QString& name, uint8_t* data, int size, int id);
  void insertVariable(VariableRefType type, int var, const QString& name);
  void insertVariableRef(int fileId, SCXOffset address, VariableRefType type,
                         int var);
  void insertLocalLabelRef(int fileId, SCXOffset address, int labelId);
  void insertString(int fileId, int stringId, const std::string& string);

  int getGameId();
  void setGameId(int gameId);

  QSqlQuery _getCommentQuery;
  QSqlQuery _setCommentQuery;
  QSqlQuery _getLabelNameQuery;
  QSqlQuery _setLabelNameQuery;
  QSqlQuery _getFilesQuery;
  QSqlQuery _insertFileQuery;
  QSqlQuery _getAllVarNamesQuery;
  QSqlQuery _getVarNameQuery;
  QSqlQuery _getVarCommentQuery;
  QSqlQuery _insertVariableQuery;
  QSqlQuery _setVarNameQuery;
  QSqlQuery _setVarCommentQuery;
  QSqlQuery _getVariableRefsQuery;
  QSqlQuery _insertVariableRefQuery;
  QSqlQuery _getLabelRefsQuery;
  QSqlQuery _insertLocalLabelRefQuery;
  QSqlQuery _getStringQuery;
  QSqlQuery _insertStringQuery;
  QSqlQuery _getGameIdQuery;
  QSqlQuery _setGameIdQuery;
  QSqlQuery _getVarByNameQuery;
  QSqlQuery _getBreakpointQuery;
  QSqlQuery _setBreakpointQuery;
};
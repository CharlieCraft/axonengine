/*
Copyright (c) 2009 AxonEngine Team

This file is part of the AxonEngine project, and may only be used, modified, 
and distributed under the terms of the AxonEngine project license, license.txt.  
By continuing to use, modify, or distribute this file you indicate that you have
read the license and understand and accept it fully.
*/


#ifndef AX_EDITOR_CONTEXT_H
#define AX_EDITOR_CONTEXT_H

namespace Axon { namespace Editor {

	class AX_API Context : public IObservable {
	public:
		typedef Dict<int, Actor*> ActorDict;

		enum ObserverFlag {
			SelectionChanged = 1,
			HistoryChanged = 2,
			StatusChanged = 4,
			TerrainMaterialEdited = 8,
			ToolChanged = 0x10,
			EnvironmentChanged = 0x20,
			ActorTransformed = 0x40,
			EverythingChanged = 0xffffffff
		};

		Context();
		virtual ~Context();

		// present
		virtual void doRender(const Render::Camera& camera, bool world = false) = 0;
		virtual void doSelect(const Render::Camera& camera, int part) = 0;
		virtual MapTerrain* getTerrain() { return 0; }

		int generateActorId();
		void addActor(Actor* a);
		void removeActor(Actor* a);
		Actor* findActor(int id);
		const ActorDict& getActorDict() { return m_actorDict; }

		Action* createAction(int type);
		Tool* createTool(int type);

		void reset();

		String getTitle() const;
		String getFilename() const;
		bool createNew();
		bool load(const String& filename);
		bool save();
		bool saveAs(const String& filename);
		bool isDirty() const { return m_isDirty; }
		bool isLoading() const { return m_isLoading; }

		// view process
		int getNumViews() const { return m_numViews; }
		View* getView(int index);
		void setActiveView(Editor::View* view) { m_activeView = view; }
		View* getActiveView() const { return m_activeView; }

		Vector3 getViewPos();

		void doAction(int action);
		void doTool(int tool);
		Tool* getTool() const { return m_tool; }
		int getToolType() const { return m_tooltype; }

		// history
		void addHistory(Action* his);
		void undo();
		void redo();
		HistoryManager* getHistory();

		// selection
		const ActorList& getSelection() { return m_selections; }
		History* setSelectionHistoried(const ActorList& elist);
		void setSelection(const ActorList& elist, bool undoable=true);
		void setSelection(Actor* actor, bool undoable=true);
		void selectNone(bool undoable=true);
		void selectAll(bool undoable=true);
		void selectInvert(bool undoable=true);
		void addSelection(const ActorList& elist, bool undoable=true);

		// properties
		void setActorProperty(const String& propName, const Variant& value);

		// state
		State* getState() const { return m_state; }
		void setState(State* val) { m_state = val; }

	protected:
		int m_maxId;
		ActorDict m_actorDict;
		bool m_isDirty;
		bool m_isLoading;

		ActionFactory* m_actionFactories[Action::MaxType];
		ToolFactory* m_toolFactories[Tool::MaxType];

		int m_numViews;
		View* m_indexedViews[Editor::View::MaxView];
		View* m_activeView;

		// tool
		int m_tooltype;
		Tool* m_tool;

		// selection
		ActorList m_selections;

		// history
		HistoryManager m_editorHistory;

		// state
		State* m_state;
	};

}} // namespace Axon::Editor

#endif

